#include "matrix_dense.h"

#include <cassert>
#include <initializer_list>
#include <stdexcept>

int main() {
    using mytrix::ComputeBackend;

    assert(mytrix::BackendConfig::is_backend_available(ComputeBackend::CPU));
    assert(mytrix::BackendConfig::is_backend_available(ComputeBackend::EIGEN));
    assert(!mytrix::BackendConfig::is_backend_available(ComputeBackend::GPU_OPENCL));

    bool rejected_opencl = false;
    try {
        mytrix::set_backend(ComputeBackend::GPU_OPENCL);
    } catch (const std::invalid_argument&) {
        rejected_opencl = true;
    }
    assert(rejected_opencl);
    assert(std::string(mytrix::BackendConfig::backend_name(ComputeBackend::OPENBLAS)) == "openblas");
#ifndef MYTRIX_ENABLE_OPENBLAS
    assert(!mytrix::BackendConfig::is_backend_available(ComputeBackend::OPENBLAS));
    bool rejected_openblas = false;
    try { mytrix::set_backend(ComputeBackend::OPENBLAS); }
    catch (const std::invalid_argument&) { rejected_openblas = true; }
    assert(rejected_openblas);
#else
    assert(mytrix::BackendConfig::is_backend_available(ComputeBackend::OPENBLAS));
    mytrix::DenseMatrix a(7, 19), b(19, 13);
    a.setRandom(); b.setRandom();
    mytrix::set_backend(ComputeBackend::OPENBLAS);
    const auto original_threads = openblas_get_num_threads();
    for (const auto threads : {1u, 2u}) {
        mytrix::set_cpu_threads(threads);
        const auto c = a.multiply(b);
        assert((static_cast<const mytrix::DenseMatrix&>(*c).data - a.data * b.data).norm() < 1e-12);
        assert(mytrix::BackendConfig::active_backend() == ComputeBackend::OPENBLAS);
        assert(openblas_get_num_threads() == original_threads);
    }
    a.transpose();
    assert(mytrix::BackendConfig::active_backend() == ComputeBackend::CPU);
    const mytrix::DenseMatrix empty(0, 3), right(3, 0);
    const auto empty_result = empty.multiply(right);
    assert(empty_result->rows() == 0 && empty_result->cols() == 0);
    mytrix::set_cpu_threads(0);
#endif

    mytrix::DenseMatrix lhs(2, 2);
    lhs.data << 1, 2, 3, 4;
    mytrix::DenseMatrix rhs(2, 2);
    rhs.data << 5, 6, 7, 8;

    mytrix::set_backend(ComputeBackend::EIGEN);
    const auto eigen_product = lhs.multiply(rhs);
    assert(mytrix::BackendConfig::active_backend() == ComputeBackend::EIGEN);
    assert((static_cast<const mytrix::DenseMatrix&>(*eigen_product).data -
            (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);

    mytrix::set_backend(ComputeBackend::GPU_AUTO);
    const auto automatic_product = lhs.multiply(rhs);
    const auto resolved = mytrix::BackendConfig::active_backend();
    assert(resolved == ComputeBackend::CPU ||
           resolved == ComputeBackend::GPU_CUDA ||
           resolved == ComputeBackend::GPU_METAL);
    assert((static_cast<const mytrix::DenseMatrix&>(*automatic_product).data -
            (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);

    for (const auto backend : {ComputeBackend::GPU_CUDA, ComputeBackend::GPU_METAL}) {
        if (!mytrix::BackendConfig::is_backend_available(backend)) continue;
        mytrix::set_backend(backend);
        const auto product = lhs.multiply(rhs);
        assert(mytrix::BackendConfig::active_backend() == backend);
        assert((static_cast<const mytrix::DenseMatrix&>(*product).data -
                (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);
    }

    mytrix::set_backend(ComputeBackend::CPU);
    return 0;
}
