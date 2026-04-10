#ifndef COOLBOX__LIBRARIES_GO_BINDINGS_ABI_LINEAR_REGRESSION_H
#define COOLBOX__LIBRARIES_GO_BINDINGS_ABI_LINEAR_REGRESSION_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxLinearRegressionModel CoolBoxLinearRegressionModel;

CoolBoxLinearRegressionModel* coolbox_fit_linear_regression(
    const double* x_flat,
    size_t rows,
    size_t cols,
    const double* y,
    const char* method,
    uint32_t iterations,
    double learning_rate,
    char** error_message);

int coolbox_predict_linear_regression(
    const CoolBoxLinearRegressionModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_predictions,
    char** error_message);

size_t coolbox_linear_regression_feature_count(
    const CoolBoxLinearRegressionModel* model);

double coolbox_linear_regression_intercept(
    const CoolBoxLinearRegressionModel* model);

double coolbox_linear_regression_weight_at(
    const CoolBoxLinearRegressionModel* model,
    size_t index);

const char* coolbox_linear_regression_method_name(
    const CoolBoxLinearRegressionModel* model);

void coolbox_free_linear_regression(
    CoolBoxLinearRegressionModel* model);

#ifdef __cplusplus
}
#endif
#endif  // COOLBOX__LIBRARIES_GO_BINDINGS_ABI_LINEAR_REGRESSION_H
