#pragma once
#include "matrix_base.h"
#include <Eigen/Dense>
namespace mytrix {

class DenseMatrix : public MatrixBase {
public:
	Eigen::MatrixXd data;
	DenseMatrix(int r, int c) : data(r, c) {}
	int rows() const override { return data.rows(); }
	int cols() const override { return data.cols(); }
	// Add more methods as needed
};

class DenseVector : public VectorBase {
public:
	Eigen::VectorXd data;
	DenseVector(int n) : data(n) {}
	int size() const override { return data.size(); }
	// Add more methods as needed
};

using Matrix = DenseMatrix;
using Vector = DenseVector;

} // namespace mytrix
