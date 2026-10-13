#pragma once
#include "matrix_base.h"
#include <Eigen/Sparse>
namespace mytrix {

class SparseMatrix : public MatrixBase {
public:
	Eigen::SparseMatrix<double> data;
	SparseMatrix(int r, int c) : data(r, c) {}
	int rows() const override { return data.rows(); }
	int cols() const override { return data.cols(); }
	// Add more methods as needed
};

} // namespace mytrix
