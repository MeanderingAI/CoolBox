#pragma once

// mytrix compatibility typedefs
#include "matrix_dense.h"
#include <vector>
namespace mytrix {
using Index = std::size_t;
using Matrix = matrix::DenseMatrix; // Use Matrix::Identity(n) and Matrix::Zero(rows, cols) for initialization

class my_vector : public std::vector<double> {
public:
	using std::vector<double>::vector;

	my_vector(size_t n, double val = 0.0) : std::vector<double>(n, val) {}

	static my_vector Zero(size_t n) {
		return my_vector(n, 0.0);
	}

	// Elementwise addition
	my_vector& operator+=(const my_vector& other) {
		for (size_t i = 0; i < this->size(); ++i) {
			(*this)[i] += other[i];
		}
		return *this;
	}

	// Scalar multiplication
	my_vector operator*(double scalar) const {
		my_vector result = *this;
		for (auto& v : result) v *= scalar;
		return result;
	}

	// Elementwise subtraction
	my_vector operator-(const my_vector& other) const {
		my_vector result = *this;
		for (size_t i = 0; i < this->size(); ++i) {
			result[i] -= other[i];
		}
		return result;
	}
};

using Vector = my_vector;
using MatrixI = matrix::DenseMatrix; // or another integer matrix type if available
// Add any additional mytrix-specific typedefs here
} // namespace mytrix