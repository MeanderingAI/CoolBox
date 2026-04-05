#ifndef COOLBOX_GO_BINDINGS_ABI_DECISION_TREE_H
#define COOLBOX_GO_BINDINGS_ABI_DECISION_TREE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxDecisionTreeModel CoolBoxDecisionTreeModel;

CoolBoxDecisionTreeModel* coolbox_create_decision_tree(
    const char* criterion,
    char** error_message);

int coolbox_decision_tree_fit(
    CoolBoxDecisionTreeModel* model,
    const int* x_flat,
    size_t rows,
    size_t cols,
    const int* y,
    int max_depth,
    char** error_message);

int coolbox_decision_tree_predict(
    const CoolBoxDecisionTreeModel* model,
    const int* sample,
    size_t feature_count,
    int* out_label,
    char** error_message);

void coolbox_free_decision_tree(CoolBoxDecisionTreeModel* model);

#ifdef __cplusplus
}
#endif

#endif
