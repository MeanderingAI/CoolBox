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

    // Test GPU_AUTO backend resolution
    mytrix::set_backend(mytrix::ComputeBackend::GPU_AUTO);
    mytrix::ComputeBackend resolved = mytrix::BackendConfig::resolve_backend();
    std::cout << "  GPU_AUTO resolves to: " << mytrix::backend_name(resolved) << std::endl;

    // Test explicit Metal selection (will fall back to CPU if not available)
    mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
    resolved = mytrix::BackendConfig::resolve_backend();
    if (resolved == mytrix::ComputeBackend::GPU_METAL) {
        std::cout << "  Metal backend: AVAILABLE and selected" << std::endl;
    } else {
        std::cout << "  Metal backend: unavailable, fell back to " << mytrix::backend_name(resolved) << std::endl;
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

    // Test multiply with Metal (or fallback)
    mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
    auto metal_mult = a.multiply(b);
    Eigen::MatrixXd metal_mult_result = static_cast<Eigen::MatrixXd>(*metal_mult);

    // Verify numerical parity (allowing for floating point rounding)
    double tolerance = 1e-10;
    bool mult_matches = (cpu_mult_result - metal_mult_result).cwiseAbs().maxCoeff() < tolerance;
    std::cout << "  Matrix multiply parity: " << (mult_matches ? "PASS" : "FAIL") << std::endl;
    assert(mult_matches);

    // Test transpose
    mytrix::set_backend(mytrix::ComputeBackend::CPU);
    auto cpu_trans = a.transpose();
    Eigen::MatrixXd cpu_trans_result = static_cast<Eigen::MatrixXd>(*cpu_trans);

    mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
    auto metal_trans = a.transpose();
    Eigen::MatrixXd metal_trans_result = static_cast<Eigen::MatrixXd>(*metal_trans);

    bool trans_matches = (cpu_trans_result - metal_trans_result).cwiseAbs().maxCoeff() < tolerance;
    std::cout << "  Matrix transpose parity: " << (trans_matches ? "PASS" : "FAIL") << std::endl;
    assert(trans_matches);

    // Test add
    mytrix::set_backend(mytrix::ComputeBackend::CPU);
    auto cpu_add = a.add(b);
    Eigen::MatrixXd cpu_add_result = static_cast<Eigen::MatrixXd>(*cpu_add);

    mytrix::set_backend(mytrix::ComputeBackend::GPU_METAL);
    auto metal_add = a.add(b);
    Eigen::MatrixXd metal_add_result = static_cast<Eigen::MatrixXd>(*metal_add);

    bool add_matches = (cpu_add_result - metal_add_result).cwiseAbs().maxCoeff() < tolerance;
    std::cout << "  Matrix add parity: " << (add_matches ? "PASS" : "FAIL") << std::endl;
    assert(add_matches);
}

void test_metal_availability() {
    std::cout << "Testing Metal backend availability..." << std::endl;

#ifdef MYTRIX_ENABLE_METAL
    bool metal_compiled = true;
    std::cout << "  Metal support compiled in: YES" << std::endl;
#else
    bool metal_compiled = false;
    std::cout << "  Metal support compiled in: NO" << std::endl;
#endif

#ifdef MYTRIX_ENABLE_METAL
    bool metal_available = mytrix::metal::MetalBackend::is_available();
#else
    bool metal_available = false;
#endif

    std::cout << "  Metal backend available at runtime: " << (metal_available ? "YES" : "NO") << std::endl;
    assert(metal_compiled == metal_available);
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
