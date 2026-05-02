// SVM bindings — uses Eigen types; provide wrapper functions for JS.
#include <emscripten/bind.h>
#include "support_vector_machine.h"
#include "linear_kernel.h"
#include "rbf_kernel.h"
#include "polynomial_kernel.h"

using namespace emscripten;

// Wrapper class that accepts JS-friendly types
class SVMWrapper {
    std::unique_ptr<SVM> svm_;
    std::unique_ptr<Kernel> kernel_;
public:
    SVMWrapper(const std::string& kernel_type, double param1 = 1.0, double param2 = 0.0, int param3 = 3) {
        if (kernel_type == "linear") kernel_ = std::make_unique<LinearKernel>();
        else if (kernel_type == "rbf") kernel_ = std::make_unique<RBFKernel>(param1);
        else if (kernel_type == "polynomial") kernel_ = std::make_unique<PolynomialKernel>(param1, param2, param3);
        else kernel_ = std::make_unique<LinearKernel>();
        svm_ = std::make_unique<SVM>(*kernel_);
    }

    void fit(const std::vector<std::vector<double>>& X_vec, const std::vector<double>& y_vec) {
        Eigen::MatrixXd X(X_vec.size(), X_vec[0].size());
        for (size_t i = 0; i < X_vec.size(); ++i)
            for (size_t j = 0; j < X_vec[i].size(); ++j)
                X(i, j) = X_vec[i][j];
        Eigen::VectorXd y = Eigen::Map<const Eigen::VectorXd>(y_vec.data(), y_vec.size());
        svm_->fit(X, y);
    }

    double predict(const std::vector<double>& sample) {
        Eigen::VectorXd s = Eigen::Map<const Eigen::VectorXd>(sample.data(), sample.size());
        return svm_->predict(s);
    }
};

EMSCRIPTEN_BINDINGS(svm_module) {
    register_vector<double>("VectorDouble_SVM");
    register_vector<std::vector<double>>("VectorVectorDouble_SVM");

    class_<SVMWrapper>("SVM")
        .constructor<std::string, double, double, int>()
        .function("fit", &SVMWrapper::fit)
        .function("predict", &SVMWrapper::predict)
    ;
}
