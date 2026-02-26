#include "support_vector_machine.h"
#include <cmath>
#include <algorithm>
#include <limits>

SVM::SVM(const Kernel& kernel) : bias_(0.0), kernel_(kernel) {}

void SVM::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    int n = X.rows();
    int max_iter = 1000;
    double C = 1.0;
    double tol = 1e-5;

    alphas_ = Eigen::VectorXd::Zero(n);

    // Simplified SMO algorithm
    for (int iter = 0; iter < max_iter; ++iter) {
        int num_changed = 0;
        for (int i = 0; i < n; ++i) {
            double Ei = 0.0;
            for (int k = 0; k < n; ++k) {
                Ei += alphas_(k) * y(k) * kernel_.calculate(X.row(k), X.row(i));
            }
            Ei += bias_ - y(i);

            if ((y(i) * Ei < -tol && alphas_(i) < C) ||
                (y(i) * Ei > tol && alphas_(i) > 0)) {
                // Pick j randomly (simplified: just use sequential)
                int j = (i + 1) % n;

                double Ej = 0.0;
                for (int k = 0; k < n; ++k) {
                    Ej += alphas_(k) * y(k) * kernel_.calculate(X.row(k), X.row(j));
                }
                Ej += bias_ - y(j);

                double alpha_i_old = alphas_(i);
                double alpha_j_old = alphas_(j);

                double L, H;
                if (y(i) != y(j)) {
                    L = std::max(0.0, alphas_(j) - alphas_(i));
                    H = std::min(C, C + alphas_(j) - alphas_(i));
                } else {
                    L = std::max(0.0, alphas_(i) + alphas_(j) - C);
                    H = std::min(C, alphas_(i) + alphas_(j));
                }
                if (std::abs(L - H) < 1e-10) continue;

                double eta = 2.0 * kernel_.calculate(X.row(i), X.row(j))
                           - kernel_.calculate(X.row(i), X.row(i))
                           - kernel_.calculate(X.row(j), X.row(j));
                if (eta >= 0) continue;

                alphas_(j) -= y(j) * (Ei - Ej) / eta;
                alphas_(j) = std::min(H, std::max(L, alphas_(j)));

                if (std::abs(alphas_(j) - alpha_j_old) < 1e-5) continue;

                alphas_(i) += y(i) * y(j) * (alpha_j_old - alphas_(j));

                double b1 = bias_ - Ei
                    - y(i) * (alphas_(i) - alpha_i_old) * kernel_.calculate(X.row(i), X.row(i))
                    - y(j) * (alphas_(j) - alpha_j_old) * kernel_.calculate(X.row(i), X.row(j));
                double b2 = bias_ - Ej
                    - y(i) * (alphas_(i) - alpha_i_old) * kernel_.calculate(X.row(i), X.row(j))
                    - y(j) * (alphas_(j) - alpha_j_old) * kernel_.calculate(X.row(j), X.row(j));

                if (alphas_(i) > 0 && alphas_(i) < C) bias_ = b1;
                else if (alphas_(j) > 0 && alphas_(j) < C) bias_ = b2;
                else bias_ = (b1 + b2) / 2.0;

                num_changed++;
            }
        }
        if (num_changed == 0) break;
    }

    // Store support vectors
    std::vector<int> sv_indices;
    for (int i = 0; i < n; ++i) {
        if (alphas_(i) > 1e-8) {
            sv_indices.push_back(i);
        }
    }

    support_vectors_.resize(sv_indices.size(), X.cols());
    support_vector_labels_.resize(sv_indices.size());
    Eigen::VectorXd sv_alphas(sv_indices.size());
    for (size_t i = 0; i < sv_indices.size(); ++i) {
        support_vectors_.row(i) = X.row(sv_indices[i]);
        support_vector_labels_(i) = y(sv_indices[i]);
        sv_alphas(i) = alphas_(sv_indices[i]);
    }
    alphas_ = sv_alphas;
}

double SVM::predict(const Eigen::VectorXd& sample) const {
    double result = bias_;
    for (int i = 0; i < support_vectors_.rows(); ++i) {
        result += alphas_(i) * support_vector_labels_(i) *
                  kernel_.calculate(support_vectors_.row(i), sample);
    }
    return (result >= 0) ? 1.0 : -1.0;
}
