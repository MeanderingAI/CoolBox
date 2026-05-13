#pragma once
#include "matrix_base.h"
#include <Eigen/Dense>
#include <memory>
#include <stdexcept>
#include <vector>
#include <initializer_list>

namespace mytrix {

class DenseMatrix : public MatrixBase {
public:
    Eigen::MatrixXd data;

    DenseMatrix() : data(0, 0) {}
    DenseMatrix(int r, int c) : data(r, c) { data.setZero(); }
    // Implicit conversion from Eigen for test/interop compatibility
    DenseMatrix(const Eigen::MatrixXd& m) : data(m) {}
    DenseMatrix(Eigen::MatrixXd&& m) : data(std::move(m)) {}
    // Flat-vector constructor: data in row-major order
    DenseMatrix(const std::vector<double>& flat, int r, int c) : data(r, c) {
        for (int i = 0; i < r; ++i)
            for (int j = 0; j < c; ++j)
                data(i, j) = flat[static_cast<std::size_t>(i * c + j)];
    }
    DenseMatrix(const std::vector<double>& flat, std::size_t r, std::size_t c)
        : DenseMatrix(flat, static_cast<int>(r), static_cast<int>(c)) {}

    // Conversion to Eigen for test/interop compatibility
    operator Eigen::MatrixXd() const { return data; }

    int rows() const override { return static_cast<int>(data.rows()); }
    int cols() const override { return static_cast<int>(data.cols()); }

    double& at(int i, int j) { return data(i, j); }
    const double& at(int i, int j) const { return data(i, j); }
    double& at(std::size_t i, std::size_t j) { return data(static_cast<int>(i), static_cast<int>(j)); }
    const double& at(std::size_t i, std::size_t j) const { return data(static_cast<int>(i), static_cast<int>(j)); }

    // Eigen-style operator() for test/interop compatibility
    double& operator()(int i, int j) { return data(i, j); }
    const double& operator()(int i, int j) const { return data(i, j); }

    static DenseMatrix Zero(int r, int c) {
        DenseMatrix m(r, c);
        m.data.setZero();
        return m;
    }

    static DenseMatrix Identity(int n) {
        DenseMatrix m(n, n);
        m.data.setIdentity();
        return m;
    }

    static DenseMatrix Identity(std::size_t n) {
        return Identity(static_cast<int>(n));
    }

    std::unique_ptr<MatrixBase> multiply(const DenseMatrix& other) const {
        auto result = std::make_unique<DenseMatrix>(rows(), other.cols());
        result->data = data * other.data;
        return result;
    }

    std::unique_ptr<MatrixBase> transpose() const {
        auto result = std::make_unique<DenseMatrix>(cols(), rows());
        result->data = data.transpose();
        return result;
    }

    std::unique_ptr<MatrixBase> add(const DenseMatrix& other) const {
        auto result = std::make_unique<DenseMatrix>(rows(), cols());
        result->data = data + other.data;
        return result;
    }

    DenseMatrix operator+(const DenseMatrix& other) const {
        DenseMatrix result(rows(), cols());
        result.data = data + other.data;
        return result;
    }

    DenseMatrix operator-(const DenseMatrix& other) const {
        DenseMatrix result(rows(), cols());
        result.data = data - other.data;
        return result;
    }

    DenseMatrix operator*(const DenseMatrix& other) const {
        DenseMatrix result(rows(), other.cols());
        result.data = data * other.data;
        return result;
    }

    // Norm of the matrix (Frobenius norm)
    double norm() const { return data.norm(); }

    // Eigen array proxy for element-wise operations (e.g. .array().isFinite().all())
    auto array() const { return data.array(); }
    auto array() { return data.array(); }
};

class DenseVector : public VectorBase {
public:
    Eigen::VectorXd data;

    DenseVector() : data(0) {}
    explicit DenseVector(int n) : data(Eigen::VectorXd::Zero(n)) {}
    DenseVector(int n, double val) : data(Eigen::VectorXd::Constant(n, val)) {}
    DenseVector(std::size_t n, double val) : data(Eigen::VectorXd::Constant(static_cast<int>(n), val)) {}
    // Implicit conversion from Eigen for test/interop compatibility
    DenseVector(const Eigen::VectorXd& v) : data(v) {}
    DenseVector(Eigen::VectorXd&& v) : data(std::move(v)) {}

    // Conversion to Eigen for test/interop compatibility
    operator Eigen::VectorXd() const { return data; }
    DenseVector(std::initializer_list<double> il) : data(static_cast<int>(il.size())) {
        int i = 0; for (double v : il) data(i++) = v;
    }
    template<typename InputIt>
    DenseVector(InputIt first, InputIt last) {
        std::vector<double> tmp(first, last);
        data.resize(static_cast<int>(tmp.size()));
        for (int i = 0; i < static_cast<int>(tmp.size()); ++i) data(i) = tmp[i];
    }

    int size() const override { return static_cast<int>(data.size()); }
    bool empty() const { return data.size() == 0; }

    // Iterators (raw pointer iterators over Eigen storage)
    double* begin() { return data.data(); }
    double* end()   { return data.data() + data.size(); }
    const double* begin() const { return data.data(); }
    const double* end()   const { return data.data() + data.size(); }

    double& operator[](int i) { return data(i); }
    const double& operator[](int i) const { return data(i); }
    double& operator[](std::size_t i) { return data(static_cast<int>(i)); }
    const double& operator[](std::size_t i) const { return data(static_cast<int>(i)); }

    double& at(int i) { return data(i); }
    const double& at(int i) const { return data(i); }
    double& at(std::size_t i) { return data(static_cast<int>(i)); }
    const double& at(std::size_t i) const { return data(static_cast<int>(i)); }

    // Eigen-style operator() for test/interop compatibility
    double& operator()(int i) { return data(i); }
    const double& operator()(int i) const { return data(i); }

    DenseVector operator-(const DenseVector& other) const {
        DenseVector result(static_cast<int>(data.size()), 0.0);
        result.data = data - other.data;
        return result;
    }

    DenseVector operator+(const DenseVector& other) const {
        DenseVector result(static_cast<int>(data.size()), 0.0);
        result.data = data + other.data;
        return result;
    }
};

using Matrix = DenseMatrix;
using Vector = DenseVector;

} // namespace mytrix

// Backward-compatibility alias: code using matrix::DenseMatrix resolves to mytrix::DenseMatrix
namespace matrix = mytrix;
