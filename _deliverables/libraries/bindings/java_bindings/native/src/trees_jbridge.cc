#include "coolbox_jbridge.h"
#include "random_forest.h"
#include "random_forest_regressor.h"
#include <memory>

namespace {
struct ForestHandle { explicit ForestHandle(int trees, int depth) : model(trees, depth) {} RandomForest model; size_t features = 0; bool fitted = false; };
struct ForestRegressorHandle { explicit ForestRegressorHandle(int trees, int depth, unsigned int seed) : model(trees, depth, seed) {} RandomForestRegressor model; size_t features = 0; bool fitted = false; };
}
extern "C" {
void* coolbox_trees_random_forest_new(int trees, int depth) {
    try { return new ForestHandle(trees, depth); } catch (...) { return nullptr; }
}
void coolbox_trees_random_forest_free(void* handle) { delete static_cast<ForestHandle*>(handle); }
int coolbox_trees_random_forest_fit(void* handle, const int* x, size_t rows, size_t cols, const int* y, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error, capacity, [&] {
        if (!handle || !y || rows == 0 || cols == 0 || !x) throw std::invalid_argument("forest and non-empty training data are required");
        std::vector<std::vector<int>> data(rows, std::vector<int>(cols));
        for (size_t r=0;r<rows;++r) std::copy_n(x+r*cols,cols,data[r].begin());
        auto& forest=*static_cast<ForestHandle*>(handle); forest.model.fit(data, std::vector<int>(y,y+rows)); forest.features=cols; forest.fitted=true;
    });
}
int coolbox_trees_random_forest_predict(void* handle, const int* sample, size_t cols, int* out, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error, capacity, [&] { if(!handle||!sample||!out) throw std::invalid_argument("forest, sample and output are required"); auto& f=*static_cast<ForestHandle*>(handle); if(!f.fitted) throw std::logic_error("random forest has not been fitted"); if(cols!=f.features) throw std::invalid_argument("sample feature count does not match fitted model"); *out=f.model.predict(std::vector<int>(sample,sample+cols)); });
}
void* coolbox_trees_random_forest_regressor_new(int trees, int depth, unsigned int seed) {
    try { return new ForestRegressorHandle(trees, depth, seed); } catch (...) { return nullptr; }
}
void coolbox_trees_random_forest_regressor_free(void* handle) { delete static_cast<ForestRegressorHandle*>(handle); }
int coolbox_trees_random_forest_regressor_fit(void* handle, const double* x, size_t rows, size_t cols, const double* y, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error, capacity, [&] { if(!handle||!y) throw std::invalid_argument("forest regressor and targets are required"); auto matrix=coolbox_jbridge::rows_of(x,rows,cols); auto& f=*static_cast<ForestRegressorHandle*>(handle); f.model.fit(matrix,std::vector<double>(y,y+rows)); f.features=cols; f.fitted=true; });
}
int coolbox_trees_random_forest_regressor_predict(void* handle, const double* sample, size_t cols, double* out, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error, capacity, [&] { if(!handle||!sample||!out) throw std::invalid_argument("forest regressor, sample and output are required"); auto& f=*static_cast<ForestRegressorHandle*>(handle); if(!f.fitted) throw std::logic_error("random forest regressor has not been fitted"); if(cols!=f.features) throw std::invalid_argument("sample feature count does not match fitted model"); *out=f.model.predict(std::vector<double>(sample,sample+cols)); });
}
}
