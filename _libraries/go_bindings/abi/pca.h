#ifndef COOLBOX_GO_BINDINGS_ABI_PCA_H
#define COOLBOX_GO_BINDINGS_ABI_PCA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxPCAModel CoolBoxPCAModel;

CoolBoxPCAModel* coolbox_create_pca(
    int n_components,
    int center,
    int scale,
    char** error_message);

int coolbox_pca_fit(
    CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_pca_transform(
    const CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_fit_transform(
    CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    size_t* out_cols,
    char** error_message);

int coolbox_pca_inverse_transform(
    const CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_components(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_explained_variance(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_explained_variance_ratio(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_singular_values(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_mean(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_scale(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

size_t coolbox_pca_component_count(const CoolBoxPCAModel* model);
size_t coolbox_pca_feature_count(const CoolBoxPCAModel* model);
int coolbox_pca_is_fitted(const CoolBoxPCAModel* model);

void coolbox_free_pca(CoolBoxPCAModel* model);

#ifdef __cplusplus
}
#endif

#endif
