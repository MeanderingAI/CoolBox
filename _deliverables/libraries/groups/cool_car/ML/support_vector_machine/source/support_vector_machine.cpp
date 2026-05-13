#include "support_vector_machine.h"
#include <cmath>
#include <algorithm>
#include <limits>

SVM::SVM(const Kernel& kernel)
    : support_vectors_(1, 1), bias_(0.0), kernel_(kernel) {}

void SVM::fit(const matrix::DenseMatrix& X, const matrix::DenseMatrix& y, SolverType solver) {
    // X: rows = samples, cols = features; y: column vector (n_samples x 1)
    if (X.rows() == 0 || y.rows() == 0 || X.rows() != y.rows() || y.cols() != 1) return;
    size_t n_samples = X.rows();
    size_t n_features = X.cols();

    if (solver == SolverType::GradientDescent) {
        // Linear SVM using mytrix types
        matrix::DenseMatrix w = matrix::DenseMatrix::Zero(1, n_features);
        double b = 0.0;
        double lr = 0.01;
        double lambda = 0.01;
        int epochs = 1000;
        for (int epoch = 0; epoch < epochs; ++epoch) {
            for (size_t i = 0; i < n_samples; ++i) {
                double y_i = y.at(i, 0);
                double wx = 0.0;
                for (size_t j = 0; j < n_features; ++j)
                    wx += w.at(0, j) * X.at(i, j);
                wx += b;
                if (y_i * wx < 1) {
                    for (size_t j = 0; j < n_features; ++j)
                        w.at(0, j) += lr * (y_i * X.at(i, j) - 2 * lambda * w.at(0, j));
                    b += lr * y_i;
                } else {
                    for (size_t j = 0; j < n_features; ++j)
                        w.at(0, j) += lr * (-2 * lambda * w.at(0, j));
                }
            }
        }
        support_vectors_ = w;
        support_vector_labels_ = {1.0};
        alphas_ = std::vector<double>(w.data().begin(), w.data().end());
        bias_ = b;
        return;
    }

    // SMO-like solver (dual, kernelized)
    size_t max_iter = 1000;
    double tol = 1e-4;
    double C = 1.0;
    alphas_.assign(n_samples, 0.0);
    bias_ = 0.0;
    std::vector<double> E(n_samples, 0.0);
    for (size_t iter = 0; iter < max_iter; ++iter) {
        bool changed = false;
        for (size_t i = 0; i < n_samples; ++i) {
            double f_i = 0.0;
            std::vector<double> xi(X.cols());
            for (size_t j = 0; j < X.cols(); ++j) xi[j] = X.data()[i * X.cols() + j];
            for (size_t k = 0; k < n_samples; ++k) {
                std::vector<double> xk(X.cols());
                for (size_t j = 0; j < X.cols(); ++j) xk[j] = X.data()[k * X.cols() + j];
                f_i += alphas_[k] * y.at(k, 0) * kernel_.calculate(xk, xi);
            }
            f_i += bias_;
            E[i] = f_i - y.at(i, 0);
            if ((y.at(i, 0)*E[i] < -tol && alphas_[i] < C) || (y.at(i, 0)*E[i] > tol && alphas_[i] > 0)) {
                size_t j = (i+1)%n_samples;
                double f_j = 0.0;
                std::vector<double> xj(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xj[jj] = X.data()[j * X.cols() + jj];
                for (size_t k = 0; k < n_samples; ++k) {
                    std::vector<double> xk(X.cols());
                    for (size_t jj = 0; jj < X.cols(); ++jj) xk[jj] = X.data()[k * X.cols() + jj];
                    f_j += alphas_[k] * y.at(k, 0) * kernel_.calculate(xk, xj);
                }
                f_j += bias_;
                E[j] = f_j - y.at(j, 0);
                double alpha_i_old = alphas_[i];
                double alpha_j_old = alphas_[j];
                double L, H;
                if (y.at(i, 0) != y.at(j, 0)) {
                    L = std::max(0.0, alphas_[j] - alphas_[i]);
                    H = std::min(C, C + alphas_[j] - alphas_[i]);
                } else {
                    L = std::max(0.0, alphas_[i] + alphas_[j] - C);
                    H = std::min(C, alphas_[i] + alphas_[j]);
                }
                if (L == H) continue;
                std::vector<double> xi2(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xi2[jj] = X.data()[i * X.cols() + jj];
                std::vector<double> xj2(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xj2[jj] = X.data()[j * X.cols() + jj];
                double eta = 2 * kernel_.calculate(xi2, xj2) - kernel_.calculate(xi2, xi2) - kernel_.calculate(xj2, xj2);
                if (eta >= 0) continue;
                alphas_[j] -= y.at(j, 0) * (E[i] - E[j]) / eta;
                if (alphas_[j] > H) alphas_[j] = H;
                else if (alphas_[j] < L) alphas_[j] = L;
                if (std::abs(alphas_[j] - alpha_j_old) < tol) continue;
                alphas_[i] += y.at(i, 0)*y.at(j, 0)*(alpha_j_old - alphas_[j]);
                double b1 = bias_ - E[i]
                    - y.at(i, 0)*(alphas_[i]-alpha_i_old)*kernel_.calculate(xi2, xi2)
                    - y.at(j, 0)*(alphas_[j]-alpha_j_old)*kernel_.calculate(xi2, xj2);
                double b2 = bias_ - E[j]
                    - y.at(i, 0)*(alphas_[i]-alpha_i_old)*kernel_.calculate(xi2, xj2)
                    - y.at(j, 0)*(alphas_[j]-alpha_j_old)*kernel_.calculate(xj2, xj2);
                if (0 < alphas_[i] && alphas_[i] < C) bias_ = b1;
                else if (0 < alphas_[j] && alphas_[j] < C) bias_ = b2;
                else bias_ = 0.5*(b1+b2);
                changed = true;
            }
        }
        if (!changed) break;
    }
    // Store support vectors and labels
    std::vector<double> flat_sv;
    support_vector_labels_.clear();
    for (size_t i = 0; i < n_samples; ++i) {
        if (alphas_[i] > tol) {
            for (size_t j = 0; j < X.cols(); ++j)
                flat_sv.push_back(X.data()[i * X.cols() + j]);
            support_vector_labels_.push_back(y.at(i, 0));
        }
    }

    support_vectors_ = matrix::DenseMatrix(flat_sv, support_vector_labels_.size(), n_features);
}


double SVM::predict(const matrix::DenseMatrix& sample) const {
    // sample: 1 x n_features
    if (support_vector_labels_.size() == 1 && support_vectors_.rows() == 1) {
        double result = 0.0;
        for (size_t j = 0; j < sample.cols(); ++j)
            result += support_vectors_.at(0, j) * sample.at(0, j);
        result += bias_;
        return result;
    }
    // Otherwise, kernel SVM
    double sum = 0.0;
    size_t n_features = sample.cols();
    std::vector<double> sample_vec(sample.data().begin(), sample.data().end());
    for (size_t i = 0; i < support_vector_labels_.size(); ++i) {
        std::vector<double> sv(support_vectors_.data().begin() + i*n_features,
                               support_vectors_.data().begin() + (i+1)*n_features);
        sum += alphas_[i] * support_vector_labels_[i] * kernel_.calculate(sv, sample_vec);
    }
    return sum + bias_;
}
