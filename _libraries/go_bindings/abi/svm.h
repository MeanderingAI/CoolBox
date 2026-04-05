#ifndef COOLBOX_GO_BINDINGS_ABI_SVM_H
#define COOLBOX_GO_BINDINGS_ABI_SVM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxSVMModel CoolBoxSVMModel;

CoolBoxSVMModel* coolbox_create_svm(
    const char* kernel_type,
    double param1,
    double param2,
    int param3,
    char** error_message);

int coolbox_svm_fit(
    CoolBoxSVMModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    const double* y,
    size_t y_count,
    char** error_message);

int coolbox_svm_predict(
    const CoolBoxSVMModel* model,
    const double* sample,
    size_t feature_count,
    double* out_value,
    char** error_message);

void coolbox_free_svm(CoolBoxSVMModel* model);

#ifdef __cplusplus
}
#endif

#endif
