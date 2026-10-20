#include "matrix_utils.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

void verify_optimization(mytrix::CpuOptimization optimization) {
    mytrix::DenseVector lhs(19);
    mytrix::DenseVector rhs(19);
    for (int index = 0; index < lhs.size(); ++index) {
        lhs(index) = static_cast<double>(index) * 0.25 - 1.0;
        rhs(index) = static_cast<double>(index % 5) - 2.0;
    }

    const double expected_dot = mytrix::dot(lhs, rhs, mytrix::CpuOptimization::Scalar);
    const double actual_dot = mytrix::dot(lhs, rhs, optimization);
    assert(std::abs(expected_dot - actual_dot) < 1e-12);

    const auto expected_add = mytrix::add(lhs, rhs, mytrix::CpuOptimization::Scalar);
    const auto actual_add = mytrix::add(lhs, rhs, optimization);
    assert((expected_add.data - actual_add.data).cwiseAbs().maxCoeff() < 1e-12);
}

void verify_unavailable_optimizations_fall_back() {
    for (const auto optimization : {mytrix::CpuOptimization::Avx2Fma,
                                    mytrix::CpuOptimization::Neon}) {
        if (!mytrix::cpu_optimization_available(optimization)) {
            assert(mytrix::selected_cpu_optimization(optimization) ==
                   mytrix::CpuOptimization::Scalar);
        }
    }
}

void verify_size_validation() {
    const mytrix::DenseVector short_vector(2);
    const mytrix::DenseVector long_vector(3);
    bool threw = false;
    try {
        (void)mytrix::dot(short_vector, long_vector);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void verify_products() {
    for (const auto& [m, k, n] : {std::tuple{1, 1, 1}, {3, 17, 9}, {8, 7, 16},
                                  {35, 131, 137}, {193, 129, 173}, {257, 67, 263},
                                  {129, 1025, 1025}}) {
        std::vector<double> a(m * k), b(k * n), expected(m * n, 0.0), actual(m * n, 100.0);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] = (static_cast<int>(i % 19) - 9) * .125;
        for (std::size_t i = 0; i < b.size(); ++i) b[i] = (static_cast<int>(i % 13) - 6) * .25;
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j)
                for (int p = 0; p < k; ++p) expected[i * n + j] += a[i * k + p] * b[p * n + j];
        for (const auto optimization : {mytrix::CpuOptimization::Scalar, mytrix::CpuOptimization::Auto,
                                        mytrix::CpuOptimization::Neon, mytrix::CpuOptimization::Avx2Fma})
            for (const auto threads : {1u, 2u, 8u, 0u}) {
                mytrix::set_cpu_threads(threads);
                mytrix::optimized_multiply(a.data(), b.data(), actual.data(), m, k, n, optimization);
                for (std::size_t i = 0; i < actual.size(); ++i)
                    assert(std::abs(actual[i] - expected[i]) < 1e-10);
            }
        mytrix::DenseMatrix left(a, m, k), right(b, k, n);
        mytrix::set_cpu_threads(2);
        const auto result = left * right;
        const auto product = left.multiply(right, {mytrix::ComputeBackend::CPU, false});
        const auto& mapped = static_cast<const mytrix::DenseMatrix&>(*product);
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j) {
                assert(std::abs(result(i, j) - expected[i * n + j]) < 1e-10);
                assert(std::abs(mapped(i, j) - expected[i * n + j]) < 1e-10);
            }
    }
    mytrix::set_cpu_threads(0);
    assert(mytrix::cpu_threads() >= 1);
}

void verify_worker_failure() {
    mytrix::set_cpu_threads(2);
    bool threw = false;
    try {
        mytrix::parallel_matrix_rows(256, 1024, 1024, [](std::size_t first, std::size_t) {
            if (first != 0) throw std::runtime_error("worker failure");
        });
    } catch (const std::runtime_error&) { threw = true; }
    assert(threw);
    mytrix::set_cpu_threads(0);
}

void verify_concurrent_products() {
    mytrix::set_cpu_threads(2);
    const std::size_t size = 513;
    std::vector<double> input(size * size), expected(size * size);
    for (std::size_t i = 0; i < input.size(); ++i) input[i] = std::sin(static_cast<double>(i));
    mytrix::optimized_multiply(input.data(), input.data(), expected.data(), size, size, size,
                               mytrix::CpuOptimization::Scalar);
    auto compute = [&] {
        std::vector<double> actual(size * size);
        for (int repeat = 0; repeat < 3; ++repeat) {
            mytrix::optimized_multiply(input.data(), input.data(), actual.data(), size, size, size);
            for (std::size_t i = 0; i < actual.size(); ++i)
                assert(std::abs(actual[i] - expected[i]) < 1e-10);
        }
    };
    std::thread other(compute);
    compute();
    other.join();
    mytrix::set_cpu_threads(0);
}

void verify_workload_scheduling() {
    mytrix::set_cpu_threads(8);
    for (const auto& [size, expected] : {std::tuple{128u, 1u}, {256u, 1u},
                                        {512u, 2u}, {1024u, 8u}}) {
        std::atomic<unsigned> calls{0};
        std::vector<unsigned> coverage(size, 0);
        mytrix::parallel_matrix_rows(size, size, size, [&](std::size_t first, std::size_t last) {
            ++calls;
            for (auto i = first; i < last; ++i) ++coverage[i];
        });
        assert(calls == expected);
        for (auto visited : coverage) assert(visited == 1);
    }
    mytrix::set_cpu_threads(1);
    unsigned calls = 0;
    mytrix::parallel_matrix_rows(1024, 1024, 1024, [&](std::size_t, std::size_t) { ++calls; });
    assert(calls == 1);
    mytrix::set_cpu_threads(0);
}

} // namespace

int main() {
    verify_optimization(mytrix::CpuOptimization::Auto);
    verify_optimization(mytrix::CpuOptimization::Scalar);
    verify_optimization(mytrix::CpuOptimization::Avx2Fma);
    verify_optimization(mytrix::CpuOptimization::Neon);
    verify_unavailable_optimizations_fall_back();
    verify_size_validation();
    verify_products();
    verify_worker_failure();
    verify_concurrent_products();
    verify_workload_scheduling();

    std::cout << "Selected mytrix CPU optimization: "
              << mytrix::cpu_optimization_name(mytrix::selected_cpu_optimization()) << '\n';
    return 0;
}