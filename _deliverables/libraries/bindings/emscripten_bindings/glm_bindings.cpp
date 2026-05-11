#include <emscripten/bind.h>
#include "linear_regression.h"

using namespace emscripten;

class LinearRegressionWrapper {
    LinearRegressionFitMethod fit_method_;
    LinearRegression lr_;
public:
    LinearRegressionWrapper()
        : fit_method_(), lr_(fit_method_) {}

    void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
        lr_.fit(X, y);
    }

    double predict(const std::vector<double>& sample) {
        return lr_.predict(sample);
    }
};

EMSCRIPTEN_BINDINGS(glm_module) {
    register_vector<double>("VectorDouble_GLM");
    register_vector<std::vector<double>>("VectorVectorDouble_GLM");

    class_<LinearRegressionWrapper>("LinearRegression")
        .constructor<>()
        .function("fit", &LinearRegressionWrapper::fit)
        .function("predict", &LinearRegressionWrapper::predict)
    ;
}
