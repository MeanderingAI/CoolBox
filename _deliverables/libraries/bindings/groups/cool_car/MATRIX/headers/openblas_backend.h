#pragma once

#include "cpu_optimizations.h"

#ifdef MYTRIX_ENABLE_OPENBLAS
#include <cblas.h>
#include <limits>
#include <stdexcept>

namespace mytrix::detail {

inline std::mutex openblas_mutex;

inline blasint openblas_dimension(std::size_t value) {
    if (value > static_cast<std::size_t>(std::numeric_limits<blasint>::max()))
        throw std::invalid_argument("matrix dimension exceeds the OpenBLAS integer range");
    return static_cast<blasint>(value);
}

inline void openblas_multiply(const double* lhs, const double* rhs, double* output,
                             std::size_t rows, std::size_t inner, std::size_t columns,
                             bool row_major) {
    const auto m = openblas_dimension(rows), n = openblas_dimension(columns);
    const auto k = openblas_dimension(inner);
    const auto threads = cpu_threads();
    if (threads > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument("CPU thread budget exceeds the OpenBLAS integer range");
    // OpenBLAS thread settings are global; serialize our calls and restore the previous setting.
    std::lock_guard<std::mutex> lock(openblas_mutex);
    const auto previous = openblas_get_num_threads();
    openblas_set_num_threads(static_cast<int>(threads));
    cblas_dgemm(row_major ? CblasRowMajor : CblasColMajor, CblasNoTrans, CblasNoTrans,
                m, n, k, 1.0, lhs, row_major ? k : m,
                rhs, row_major ? n : k, 0.0, output, row_major ? n : m);
    openblas_set_num_threads(previous);
}

} // namespace mytrix::detail
#endif
