#pragma once
#include "cpu_optimizations.h"
#include "matrix_dense.h"
#include "matrix_sparse.h"
#include <algorithm>
#include <stdexcept>
namespace mytrix {

inline double dot(const DenseVector& a, const DenseVector& b,
                  CpuOptimization optimization = CpuOptimization::Auto) {
	if (a.size() != b.size()) {
		throw std::invalid_argument("dot: vector size mismatch");
	}
	return optimized_dot(a.data.data(), b.data.data(), static_cast<std::size_t>(a.size()), optimization);
}

inline DenseVector add(const DenseVector& a, const DenseVector& b,
                       CpuOptimization optimization = CpuOptimization::Auto) {
	if (a.size() != b.size()) {
		throw std::invalid_argument("add: vector size mismatch");
	}
	DenseVector result(a.size());
	optimized_add(a.data.data(), b.data.data(), result.data.data(),
	              static_cast<std::size_t>(a.size()), optimization);
	return result;
}

// Add more utility functions as needed

} // namespace mytrix
