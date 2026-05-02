// PCA bindings — wraps Eigen types for JS.
#include <emscripten/bind.h>
#include "pca.h"

using namespace emscripten;
using namespace dimensionality_reduction;

class PCAWrapper {
    PCA pca_;
public:
    PCAWrapper(int n_components = 2, bool center = true, bool scale = false)
        : pca_(n_components, center, scale) {}

    void fit(const std::vector<std::vector<double>>& X_vec) {
        if (X_vec.empty()) return;
        Eigen::MatrixXd X(X_vec.size(), X_vec[0].size());
        for (size_t i = 0; i < X_vec.size(); ++i)
            for (size_t j = 0; j < X_vec[i].size(); ++j)
                X(i, j) = X_vec[i][j];
        pca_.fit(X);
    }

    std::vector<std::vector<double>> transform(const std::vector<std::vector<double>>& X_vec) {
        Eigen::MatrixXd X(X_vec.size(), X_vec[0].size());
        for (size_t i = 0; i < X_vec.size(); ++i)
            for (size_t j = 0; j < X_vec[i].size(); ++j)
                X(i, j) = X_vec[i][j];
        Eigen::MatrixXd result = pca_.transform(X);
        std::vector<std::vector<double>> out(result.rows());
        for (int i = 0; i < result.rows(); ++i) {
            out[i].resize(result.cols());
            for (int j = 0; j < result.cols(); ++j)
                out[i][j] = result(i, j);
        }
        return out;
    }
};

EMSCRIPTEN_BINDINGS(dimensionality_reduction_module) {
    register_vector<double>("VectorDouble_DR");
    register_vector<std::vector<double>>("VectorVectorDouble_DR");

    class_<PCAWrapper>("PCA")
        .constructor<int, bool, bool>()
        .function("fit", &PCAWrapper::fit)
        .function("transform", &PCAWrapper::transform)
    ;
}
