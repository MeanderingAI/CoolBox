#include <iostream>
#include <cassert>
#include <cmath>

#include "_deliverables/libraries/groups/cool_car/MATRIX/headers/matrix_dense.h"
#include "_deliverables/libraries/groups/cool_car/MATRIX/headers/backend_config.h"

#ifdef MYTRIX_ENABLE_METAL
#include "_deliverables/libraries/groups/cool_car/MATRIX/headers/metal_backend.h"
#endif

void test_backend_selection() {
    std::cout << "Testing backend selection..." << std::endl;

    // Test CPU backend
    mytrix::set_backend(mytrix::ComputeBackend::CPU);
    assert(mytrix::BackendConfig::resolve_backend() == mytrix::ComputeBackend::CPU);
    std::cout << "  CPU backend: PASS" << std::endl;

    mytrix::set_backend(mytrix::ComputeBackend::BOOST);
    assert(mytrix::BackendConfig::resolve_backend() == mytrix::ComputeBackend::BOOST);
    std::cout << "  BOOST backend: PASS" << std::endl;

    mytrix::set_backend(mytrix::ComputeBackend::EIGEN);
    assert(mytrix::BackendConfig::resolve_backend() == mytrix::ComputeBackend::EIGEN);
    std::cout << "  EIGEN backend: PASS" << std::endl;

    assert(!mytrix::BackendConfig::is_backend_available(
        mytrix::ComputeBackend::GPU_OPENCL));
    bool opencl_rejected = false;
    try {
        mytrix::set_backend(mytrix::ComputeBackend::GPU_OPENCL);
    } catch (const std::invalid_argument&) {
        opencl_rejected = true;
    }
    assert(opencl_rejected);

    // Test GPU_AUTO backend resolution
    mytrix::set_backend(mytrix::ComputeBackend::GPU_AUTO);
    mytrix::ComputeBackend resolved = mytrix::BackendConfig::resolve_backend();
    std::cout << "  GPU_AUTO resolves to: " << mytrix::backend_name(resolved) << std::endl;

    // Explicit selection must reject unavailable backends rather than silently
    // running the CPU fallback.
    if (mytrix::BackendConfig::is_backend_available(mytrix::ComputeBackend::GPU_METAL)) {
        mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
        resolved = mytrix::BackendConfig::resolve_backend();
        assert(resolved == mytrix::ComputeBackend::GPU_METAL);
        std::cout << "  Metal backend: AVAILABLE and selected" << std::endl;
    } else {
        bool rejected = false;
        try {
            mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        assert(rejected);
        std::cout << "  Metal backend: unavailable and correctly rejected" << std::endl;
    }

    mytrix::set_boost_enabled(true);
    assert(mytrix::boost_enabled());
    resolved = mytrix::BackendConfig::resolve_backend(mytrix::ComputeBackend::CPU, true);
    assert(resolved == mytrix::ComputeBackend::BOOST);
    std::cout << "  Boosted CPU request resolves to: " << mytrix::backend_name(resolved) << std::endl;
    mytrix::set_boost_enabled(false);
}

void test_matrix_operations_parity() {
    std::cout << "Testing matrix operation parity between backends..." << std::endl;

    // Create test matrices
    mytrix::DenseMatrix a(3, 3);
    a.data << 1, 2, 3,
              4, 5, 6,
              7, 8, 9;

    mytrix::DenseMatrix b(3, 3);
    b.data << 9, 8, 7,
              6, 5, 4,
              3, 2, 1;

    // Test multiply with CPU backend
    mytrix::set_backend(mytrix::ComputeBackend::CPU);
    auto cpu_mult = a.multiply(b);
    Eigen::MatrixXd cpu_mult_result = static_cast<Eigen::MatrixXd>(*cpu_mult);

    // Test multiply with Eigen.
    mytrix::set_backend(mytrix::ComputeBackend::EIGEN);
    auto eigen_mult = a.multiply(b);
    Eigen::MatrixXd eigen_mult_result = static_cast<Eigen::MatrixXd>(*eigen_mult);

    // Verify numerical parity (allowing for floating point rounding)
    double tolerance = 1e-10;
    bool mult_matches = (cpu_mult_result - eigen_mult_result).cwiseAbs().maxCoeff() < tolerance;
    std::cout << "  CPU/Eigen multiply parity: " << (mult_matches ? "PASS" : "FAIL") << std::endl;
    assert(mult_matches);

    const bool metal_available =
        mytrix::BackendConfig::is_backend_available(mytrix::ComputeBackend::GPU_METAL);
    if (metal_available) {
        mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
        auto metal_mult = a.multiply(b);
        auto metal_trans = a.transpose();
        auto metal_add = a.add(b);
        assert((cpu_mult_result -
                static_cast<Eigen::MatrixXd>(*metal_mult)).cwiseAbs().maxCoeff() < tolerance);
        assert((a.data.transpose() -
                static_cast<Eigen::MatrixXd>(*metal_trans)).cwiseAbs().maxCoeff() < tolerance);
        assert(((a.data + b.data) -
                static_cast<Eigen::MatrixXd>(*metal_add)).cwiseAbs().maxCoeff() < tolerance);
    }
}

void test_metal_availability() {
    std::cout << "Testing Metal backend availability..." << std::endl;

    const bool metal_available =
        mytrix::BackendConfig::is_backend_available(mytrix::ComputeBackend::GPU_METAL);
    std::cout << "  Metal backend available at runtime: "
              << (metal_available ? "YES" : "NO") << std::endl;
}

int main() {
    std::cout << "=== Mytrix Metal Backend Tests ===" << std::endl << std::endl;

    try {
        test_backend_selection();
        std::cout << std::endl;

        test_metal_availability();
        std::cout << std::endl;

        test_matrix_operations_parity();
        std::cout << std::endl;

        std::cout << "=== All tests passed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
