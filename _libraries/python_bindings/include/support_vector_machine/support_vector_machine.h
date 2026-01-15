#ifndef SVM_H
#define SVM_H

#include <vector>
#include <iostream>
#include <Eigen/Dense>
#include <kernel.h>

class SVM {
public:
    SVM(const Kernel& kernel);
    ~SVM() = default;

    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    double predict(const Eigen::VectorXd& sample) const;

private:
    Eigen::MatrixXd support_vectors_;
    Eigen::VectorXd support_vector_labels_;
    Eigen::VectorXd alphas_;
    double bias_;
    const Kernel& kernel_;
};

#endif // SVM_H