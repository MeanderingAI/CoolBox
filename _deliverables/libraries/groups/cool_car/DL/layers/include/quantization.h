#ifndef QUANTIZATION_H
#define QUANTIZATION_H

#include "tensor.h"

#include <cstdint>
#include <vector>

namespace ml {
namespace deep_learning {

struct QuantizationParameters {
    double scale = 1.0;
    std::int32_t zero_point = 0;
};

class QuantizedTensor {
public:
    QuantizedTensor() = default;
    QuantizedTensor(std::vector<size_t> shape,
                    std::vector<std::int8_t> data,
                    QuantizationParameters parameters);

    const std::vector<size_t>& shape() const { return shape_; }
    const std::vector<std::int8_t>& data() const { return data_; }
    std::vector<std::int8_t>& data() { return data_; }
    std::size_t size() const { return data_.size(); }
    QuantizationParameters parameters() const { return parameters_; }

private:
    std::vector<size_t> shape_;
    std::vector<std::int8_t> data_;
    QuantizationParameters parameters_;
};

QuantizationParameters calculate_symmetric_quantization_parameters(const Tensor& tensor);
QuantizedTensor quantize_tensor(const Tensor& tensor,
                               QuantizationParameters parameters = {});
Tensor dequantize_tensor(const QuantizedTensor& tensor);

} // namespace deep_learning
} // namespace ml

#endif // QUANTIZATION_H