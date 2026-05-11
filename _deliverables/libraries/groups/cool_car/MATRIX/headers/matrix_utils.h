#pragma once
#include "matrix_dense.h"
#include "matrix_sparse.h"
#include <algorithm>
namespace mytrix {

inline double dot(const DenseVector& a, const DenseVector& b) {
	return a.data.dot(b.data);
}

inline DenseVector add(const DenseVector& a, const DenseVector& b) {
	return DenseVector((a.data + b.data).eval());
}

// Add more utility functions as needed

} // namespace mytrix
