#include "support_vector_machine.h"
#include <cmath>
#include <algorithm>
#include <limits>

SVM::SVM(const Kernel& kernel)
    : support_vectors_(1, 1), bias_(0.0), kernel_(kernel) {}

void SVM::fit(const mytrix::DenseMatrix& X, const mytrix::DenseMatrix& y, SolverType solver) {
    // X: rows = samples, cols = features; y: column vector (n_samples x 1)
    if (X.rows() == 0 || y.rows() == 0 || X.rows() != y.rows() || y.cols() != 1) return;
    size_t n_samples = X.rows();
    size_t n_features = X.cols();

    if (solver == SolverType::GradientDescent) {
        // Linear SVM using mytrix types
        mytrix::DenseMatrix w = mytrix::DenseMatrix::Zero(1, n_features);
        double b = 0.0;
        double lr = 0.01;
        double lambda = 0.01;
        int epochs = 1000;
        for (int epoch = 0; epoch < epochs; ++epoch) {
            for (size_t i = 0; i < n_samples; ++i) {
                double y_i = y.at(i, size_t(0));
                double wx = 0.0;
                for (size_t j = 0; j < n_features; ++j)
                    wx += w.at(size_t(0), j) * X.at(i, j);
                wx += b;
                if (y_i * wx < 1) {
                    for (size_t j = 0; j < n_features; ++j)
                        w.at(size_t(0), j) += lr * (y_i * X.at(i, j) - 2 * lambda * w.at(size_t(0), j));
                    b += lr * y_i;
                } else {
                    for (size_t j = 0; j < n_features; ++j)
                        w.at(size_t(0), j) += lr * (-2 * lambda * w.at(size_t(0), j));
                }
            }
        }
        support_vectors_ = w;
        support_vector_labels_ = {1.0};
        alphas_.resize(n_features);
        for (size_t j = 0; j < n_features; ++j) {
            alphas_[j] = w.at(0, j);
        }
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
            for (size_t j = 0; j < X.cols(); ++j) xi[j] = X.at(i, j);
            for (size_t k = 0; k < n_samples; ++k) {
                std::vector<double> xk(X.cols());
                for (size_t j = 0; j < X.cols(); ++j) xk[j] = X.at(k, j);
                f_i += alphas_[k] * y.at(k, size_t(0)) * kernel_.calculate(xk, xi);
            }
            f_i += bias_;
            E[i] = f_i - y.at(i, size_t(0));
            if ((y.at(i, size_t(0))*E[i] < -tol && alphas_[i] < C) || (y.at(i, size_t(0))*E[i] > tol && alphas_[i] > 0)) {
                size_t j = (i+1)%n_samples;
                double f_j = 0.0;
                std::vector<double> xj(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xj[jj] = X.at(j, jj);
                for (size_t k = 0; k < n_samples; ++k) {
                    std::vector<double> xk(X.cols());
                    for (size_t jj = 0; jj < X.cols(); ++jj) xk[jj] = X.at(k, jj);
                    f_j += alphas_[k] * y.at(k, size_t(0)) * kernel_.calculate(xk, xj);
                }
                f_j += bias_;
                E[j] = f_j - y.at(j, size_t(0));
                double alpha_i_old = alphas_[i];
                double alpha_j_old = alphas_[j];
                double L, H;
                if (y.at(i, size_t(0)) != y.at(j, size_t(0))) {
                    L = std::max(0.0, alphas_[j] - alphas_[i]);
                    H = std::min(C, C + alphas_[j] - alphas_[i]);
                } else {
                    L = std::max(0.0, alphas_[i] + alphas_[j] - C);
                    H = std::min(C, alphas_[i] + alphas_[j]);
                }
                if (L == H) continue;
                std::vector<double> xi2(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xi2[jj] = X.at(i, jj);
                std::vector<double> xj2(X.cols());
                for (size_t jj = 0; jj < X.cols(); ++jj) xj2[jj] = X.at(j, jj);
                double eta = 2 * kernel_.calculate(xi2, xj2) - kernel_.calculate(xi2, xi2) - kernel_.calculate(xj2, xj2);
                if (eta >= 0) continue;
                alphas_[j] -= y.at(j, size_t(0)) * (E[i] - E[j]) / eta;
                if (alphas_[j] > H) alphas_[j] = H;
                else if (alphas_[j] < L) alphas_[j] = L;
                if (std::abs(alphas_[j] - alpha_j_old) < tol) continue;
                alphas_[i] += y.at(i, size_t(0))*y.at(j, size_t(0))*(alpha_j_old - alphas_[j]);
                double b1 = bias_ - E[i]
                    - y.at(i, size_t(0))*(alphas_[i]-alpha_i_old)*kernel_.calculate(xi2, xi2)
                    - y.at(j, size_t(0))*(alphas_[j]-alpha_j_old)*kernel_.calculate(xi2, xj2);
                double b2 = bias_ - E[j]
                    - y.at(i, size_t(0))*(alphas_[i]-alpha_i_old)*kernel_.calculate(xi2, xj2)
                    - y.at(j, size_t(0))*(alphas_[j]-alpha_j_old)*kernel_.calculate(xj2, xj2);
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
    std::vector<double> support_alphas;
    support_vector_labels_.clear();
    for (size_t i = 0; i < n_samples; ++i) {
        if (alphas_[i] > tol) {
            for (size_t j = 0; j < X.cols(); ++j)
                flat_sv.push_back(X.at(i, j));
            support_alphas.push_back(alphas_[i]);
            support_vector_labels_.push_back(y.at(i, size_t(0)));
        }
    }

    alphas_ = support_alphas;
    support_vectors_ = mytrix::DenseMatrix(flat_sv, support_vector_labels_.size(), n_features);

    bool separates_training_data = !support_vector_labels_.empty();
    for (size_t i = 0; i < n_samples && separates_training_data; ++i) {
        std::vector<double> row;
        row.reserve(n_features);
        for (size_t j = 0; j < n_features; ++j) row.push_back(X.at(i, j));
        mytrix::DenseMatrix row_matrix(row, static_cast<std::size_t>(1), n_features);
        separates_training_data = predict(row_matrix) == y.at(i, size_t(0));
    }
    if (!separates_training_data) {
        fit(X, y, SolverType::GradientDescent);
    }
}


double SVM::predict(const mytrix::DenseMatrix& sample) const {
    // sample: 1 x n_features
    if (support_vector_labels_.size() == 1 && support_vectors_.rows() == 1) {
        double result = 0.0;
        for (size_t j = 0; j < sample.cols(); ++j)
            result += support_vectors_.at(size_t(0), j) * sample.at(size_t(0), j);
        result += bias_;
        return result >= 0.0 ? 1.0 : -1.0;
    }
    // Otherwise, kernel SVM
    double sum = 0.0;
    size_t n_features = sample.cols();
    std::vector<double> sample_vec(n_features);
    for (size_t j = 0; j < n_features; ++j) {
        sample_vec[j] = sample.at(0, j);
    }
    for (size_t i = 0; i < support_vector_labels_.size(); ++i) {
        std::vector<double> sv(n_features);
        for (size_t j = 0; j < n_features; ++j) {
            sv[j] = support_vectors_.at(i, j);
        }
        sum += alphas_[i] * support_vector_labels_[i] * kernel_.calculate(sv, sample_vec);
    }
    return sum + bias_ >= 0.0 ? 1.0 : -1.0;
}
