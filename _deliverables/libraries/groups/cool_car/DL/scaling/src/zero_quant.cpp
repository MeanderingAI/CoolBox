#include "zero_quant.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

int32_t quant_max(int bits) {
    return (1 << (bits - 1)) - 1;
}

void validate_bits(int bits) {
    if (bits < 2 || bits > 16) {
        throw std::invalid_argument("zero_quant: bits must be in [2, 16]");
    }
}

} // namespace

double QuantizedTensor::storage_bytes() const {
    const double payload = static_cast<double>(values.size()) * static_cast<double>(bits) / 8.0;
    return payload + static_cast<double>(scales.size()) * sizeof(float);
}

QuantizedTensor group_wise_quantize(const Tensor& weights, size_t group_size, int bits) {
    validate_bits(bits);
    if (group_size == 0) {
        throw std::invalid_argument("group_wise_quantize: group_size must be > 0");
    }

    QuantizedTensor result;
    result.shape = weights.shape();
    result.group_size = group_size;
    result.bits = bits;
    result.values.resize(weights.size());

    const int32_t q_max = quant_max(bits);
    for (size_t start = 0; start < weights.size(); start += group_size) {
        const size_t end = std::min(start + group_size, weights.size());

        double max_abs = 0.0;
        for (size_t i = start; i < end; ++i) {
            max_abs = std::max(max_abs, std::abs(weights.data()[i]));
        }
        const double scale = max_abs > 0.0 ? max_abs / static_cast<double>(q_max) : 1.0;
        result.scales.push_back(scale);

        for (size_t i = start; i < end; ++i) {
            const double scaled = std::llround(weights.data()[i] / scale);
            result.values[i] = static_cast<int32_t>(
                std::clamp(scaled, static_cast<double>(-q_max), static_cast<double>(q_max)));
        }
    }
    return result;
}

QuantizedTensor token_wise_quantize(const Tensor& activations, int bits) {
    validate_bits(bits);
    if (activations.shape().size() != 2) {
        throw std::invalid_argument("token_wise_quantize: activations must be [tokens, hidden]");
    }
    // One scale per token row, computed dynamically from that row's range.
    const size_t hidden = activations.shape()[1];
    return group_wise_quantize(activations, hidden, bits);
}

Tensor dequantize(const QuantizedTensor& quantized) {
    Tensor result(quantized.shape, 0.0);
    if (result.size() != quantized.values.size()) {
        throw std::invalid_argument("dequantize: shape does not match the quantized payload");
    }
    for (size_t i = 0; i < quantized.values.size(); ++i) {
        const double scale = quantized.scales[i / quantized.group_size];
        result.data()[i] = static_cast<double>(quantized.values[i]) * scale;
    }
    return result;
}

Tensor fake_quantize(const Tensor& tensor, size_t group_size, int bits) {
    return dequantize(group_wise_quantize(tensor, group_size, bits));
}

double quantization_error(const Tensor& original, const Tensor& reconstructed) {
    if (original.size() != reconstructed.size()) {
        throw std::invalid_argument("quantization_error: size mismatch");
    }
    if (original.size() == 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (size_t i = 0; i < original.size(); ++i) {
        const double difference = original.data()[i] - reconstructed.data()[i];
        sum += difference * difference;
    }
    return sum / static_cast<double>(original.size());
}

double memory_reduction(const QuantizedTensor& quantized) {
    const double fp16_bytes = static_cast<double>(quantized.values.size()) * 2.0;
    const double bytes = quantized.storage_bytes();
    return bytes > 0.0 ? fp16_bytes / bytes : 0.0;
}

LayerwiseKnowledgeDistiller::LayerwiseKnowledgeDistiller()
    : LayerwiseKnowledgeDistiller(Config()) {}

LayerwiseKnowledgeDistiller::LayerwiseKnowledgeDistiller(Config config) : config_(config) {
    if (config_.learning_rate <= 0.0) {
        throw std::invalid_argument("LayerwiseKnowledgeDistiller: learning_rate must be > 0");
    }
}

Tensor LayerwiseKnowledgeDistiller::distill(const Tensor& teacher_weight,
                                            const std::vector<Tensor>& calibration_inputs) const {
    if (teacher_weight.shape().size() != 2) {
        throw std::invalid_argument("LayerwiseKnowledgeDistiller: weight must be 2D");
    }
    loss_history_.clear();

    Tensor student = teacher_weight.clone();
    const size_t rows = teacher_weight.shape()[0];
    const size_t cols = teacher_weight.shape()[1];

    for (size_t iteration = 0; iteration < config_.iterations; ++iteration) {
        Tensor quantized = fake_quantize(student, config_.group_size, config_.bits);
        Tensor gradient(student.shape(), 0.0);
        double loss = 0.0;
        size_t elements = 0;

        for (const Tensor& input : calibration_inputs) {
            if (input.shape().size() != 2 || input.shape()[1] != rows) {
                throw std::invalid_argument(
                    "LayerwiseKnowledgeDistiller: calibration input must be [tokens, weight_rows]");
            }
            const Tensor teacher_output = input.matmul(teacher_weight);
            const Tensor student_output = input.matmul(quantized);
            const size_t tokens = input.shape()[0];

            for (size_t t = 0; t < tokens; ++t) {
                for (size_t c = 0; c < cols; ++c) {
                    const double difference =
                        student_output.data()[t * cols + c] - teacher_output.data()[t * cols + c];
                    loss += difference * difference;
                    ++elements;
                    // Straight-through estimator: the quantizer passes the
                    // gradient unchanged to the underlying fp weight.
                    const double scaled = 2.0 * difference;
                    for (size_t r = 0; r < rows; ++r) {
                        gradient.data()[r * cols + c] += scaled * input.data()[t * rows + r];
                    }
                }
            }
        }

        if (elements == 0) {
            break;
        }
        loss /= static_cast<double>(elements);
        loss_history_.push_back(loss);

        const double normalizer = config_.learning_rate / static_cast<double>(elements);
        for (size_t i = 0; i < student.size(); ++i) {
            student.data()[i] -= normalizer * gradient.data()[i];
        }
    }

    return student;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
