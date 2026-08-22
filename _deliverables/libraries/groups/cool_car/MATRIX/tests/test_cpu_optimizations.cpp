#include "matrix_utils.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

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

} // namespace

int main() {
    verify_optimization(mytrix::CpuOptimization::Auto);
    verify_optimization(mytrix::CpuOptimization::Scalar);
    verify_optimization(mytrix::CpuOptimization::Avx2Fma);
    verify_optimization(mytrix::CpuOptimization::Neon);
    verify_unavailable_optimizations_fall_back();
    verify_size_validation();

    std::cout << "Selected mytrix CPU optimization: "
              << mytrix::cpu_optimization_name(mytrix::selected_cpu_optimization()) << '\n';
    return 0;
}