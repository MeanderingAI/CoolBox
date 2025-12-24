#include "ml/support_vector_machine/support_vector_machine.h"

SVM::SVM(const Kernel& kernel) : kernel_(kernel) {}

void SVM::fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) {
    if (X.rows() == 0 || y.rows() == 0 || X.rows() != y.rows()) {
        throw std::invalid_argument("Invalid input data for fitting.");
    }

    std::cout << "SVM fit method called. Using a placeholder for training." << std::endl;

    // The support vectors and alphas are now Eigen types.
    support_vectors_ = X;
    support_vector_labels_ = y;
    alphas_.setConstant(X.rows(), 0.5); // Example alpha values
    bias_ = 0.1;
}

double SVM::predict(const Eigen::VectorXd& sample) const {
    double decision_function_value = 0.0;
    for (int i = 0; i < support_vectors_.rows(); ++i) {
        // Extract a row from the matrix to pass to the kernel function
        Eigen::VectorXd sv = support_vectors_.row(i);
        double kernel_value = kernel_.calculate(sv, sample);
        decision_function_value += alphas_(i) * support_vector_labels_(i) * kernel_value;
    }
    decision_function_value += bias_;

    return (decision_function_value >= 0.0) ? 1.0 : -1.0;
}