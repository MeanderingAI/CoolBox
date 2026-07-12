#include "quantization.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ml {
namespace deep_learning {

namespace {

constexpr std::int32_t kQuantMin = -128;
constexpr std::int32_t kQuantMax = 127;

std::int8_t clamp_to_int8(std::int32_t value) {
    value = std::max(kQuantMin, std::min(kQuantMax, value));
    return static_cast<std::int8_t>(value);
}

} // namespace

QuantizedTensor::QuantizedTensor(std::vector<size_t> shape,
                                 std::vector<std::int8_t> data,
                                 QuantizationParameters parameters)
    : shape_(std::move(shape)), data_(std::move(data)), parameters_(parameters) {}

QuantizationParameters calculate_symmetric_quantization_parameters(const Tensor& tensor) {
    double max_abs = 0.0;
    for (double value : tensor.data()) {
        max_abs = std::max(max_abs, std::abs(value));
    }

    QuantizationParameters parameters;
    parameters.zero_point = 0;
    parameters.scale = max_abs > 0.0 ? (max_abs / static_cast<double>(kQuantMax)) : 1.0;
    return parameters;
}

QuantizedTensor quantize_tensor(const Tensor& tensor, QuantizationParameters parameters) {
    if (parameters.scale <= 0.0) {
        parameters = calculate_symmetric_quantization_parameters(tensor);
    }

    std::vector<std::int8_t> quantized;
    quantized.reserve(tensor.size());

    for (double value : tensor.data()) {
        const double scaled = value / parameters.scale;
        const std::int32_t rounded = static_cast<std::int32_t>(std::lrint(scaled)) + parameters.zero_point;
        quantized.push_back(clamp_to_int8(rounded));
    }

    return QuantizedTensor(tensor.shape(), std::move(quantized), parameters);
}

Tensor dequantize_tensor(const QuantizedTensor& tensor) {
    if (tensor.parameters().scale <= 0.0) {
        throw std::invalid_argument("quantization scale must be positive");
    }

    std::vector<double> values;
    values.reserve(tensor.size());

    for (std::int8_t value : tensor.data()) {
        const std::int32_t centered = static_cast<std::int32_t>(value) - tensor.parameters().zero_point;
        values.push_back(static_cast<double>(centered) * tensor.parameters().scale);
    }

    return Tensor(tensor.shape(), values);
}

} // namespace deep_learning
} // namespace ml