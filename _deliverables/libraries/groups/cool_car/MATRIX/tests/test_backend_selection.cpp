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

    mytrix::DenseMatrix lhs(2, 2);
    lhs.data << 1, 2, 3, 4;
    mytrix::DenseMatrix rhs(2, 2);
    rhs.data << 5, 6, 7, 8;

    mytrix::set_backend(ComputeBackend::EIGEN);
    const auto eigen_product = lhs.multiply(rhs);
    assert(mytrix::BackendConfig::active_backend() == ComputeBackend::EIGEN);
    assert((static_cast<Eigen::MatrixXd>(*eigen_product) -
            (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);

    mytrix::set_backend(ComputeBackend::GPU_AUTO);
    const auto automatic_product = lhs.multiply(rhs);
    const auto resolved = mytrix::BackendConfig::active_backend();
    assert(resolved == ComputeBackend::CPU ||
           resolved == ComputeBackend::GPU_CUDA ||
           resolved == ComputeBackend::GPU_METAL);
    assert((static_cast<Eigen::MatrixXd>(*automatic_product) -
            (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);

    for (const auto backend : {ComputeBackend::GPU_CUDA, ComputeBackend::GPU_METAL}) {
        if (!mytrix::BackendConfig::is_backend_available(backend)) continue;
        mytrix::set_backend(backend);
        const auto product = lhs.multiply(rhs);
        assert(mytrix::BackendConfig::active_backend() == backend);
        assert((static_cast<Eigen::MatrixXd>(*product) -
                (lhs.data * rhs.data)).cwiseAbs().maxCoeff() == 0.0);
    }

    mytrix::set_backend(ComputeBackend::CPU);
    return 0;
}
