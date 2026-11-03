#ifndef ML_DEEP_LEARNING_SCALING_SAMSUNG_QUANTIZATION_INTERNAL_H
#define ML_DEEP_LEARNING_SCALING_SAMSUNG_QUANTIZATION_INTERNAL_H

#include "tensor.h"

#include <cstddef>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace samsung_internal {

struct Svd {
    Tensor u;
    std::vector<double> singular_values;
    Tensor vt;
};

void validate_matrix(const Tensor& matrix, const char* method);
Svd truncated_svd(const Tensor& matrix, size_t rank);
Tensor matmul(const Tensor& left, const Tensor& right);
Tensor transpose(const Tensor& matrix);
Tensor identity(size_t size);
Tensor sign(const Tensor& matrix);
Tensor rank_one_magnitude(const Tensor& matrix, size_t iterations);
Tensor solve(const Tensor& coefficients, const Tensor& rhs);
double squared_error(const Tensor& a, const Tensor& b);

} // namespace samsung_internal
} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif
