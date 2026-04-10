#pragma once

#include <Eigen/Dense>
#include <Eigen/SVD>

namespace mytrix {

using Index = Eigen::Index;
using MatrixXd = Eigen::MatrixXd;
using MatrixXf = Eigen::MatrixXf;
using VectorXd = Eigen::VectorXd;
using VectorXf = Eigen::VectorXf;
using RowVectorXd = Eigen::RowVectorXd;
using RowVectorXf = Eigen::RowVectorXf;

template <typename Scalar, int Rows = Eigen::Dynamic, int Cols = Eigen::Dynamic>
using Matrix = Eigen::Matrix<Scalar, Rows, Cols>;

template <typename MatrixType>
using JacobiSVD = Eigen::JacobiSVD<MatrixType>;

} // namespace mytrix