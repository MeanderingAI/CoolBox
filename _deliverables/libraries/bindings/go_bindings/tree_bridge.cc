#include <decision_tree.h>
#include <decision_tree_regressor.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

struct ClassifierHandle {
    explicit ClassifierHandle(int criterion)
        : model(criterion == 0 ? SplitCriterion::GINI : SplitCriterion::ENTROPY) {}
    DecisionTree model;
    size_t feature_count = 0;
    bool fitted = false;
};

struct RegressorHandle {
    DecisionTreeRegressor model;
    size_t feature_count = 0;
    bool fitted = false;
};

void set_error(char* buffer, size_t capacity, const char* message) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }
    const size_t length = std::min(capacity - 1, std::strlen(message));
    std::memcpy(buffer, message, length);
    buffer[length] = '\0';
}

template <typename Function>
int invoke(char* error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return 0;
}

template <typename Value>
std::vector<std::vector<Value>> make_matrix(const Value* values,
                                            size_t rows,
                                            size_t columns) {
    if (values == nullptr || rows == 0 || columns == 0 ||
        rows > std::numeric_limits<size_t>::max() / columns) {
        throw std::invalid_argument("feature matrix must be non-empty and have valid dimensions");
    }
    std::vector<std::vector<Value>> matrix(rows, std::vector<Value>(columns));
    for (size_t row = 0; row < rows; ++row) {
        std::copy_n(values + row * columns, columns, matrix[row].begin());
    }
    return matrix;
}

} // namespace

extern "C" {

void* coolbox_tree_classifier_new(int criterion) {
    if (criterion != 0 && criterion != 1) {
        return nullptr;
    }
    try {
        return new ClassifierHandle(criterion);
    } catch (...) {
        return nullptr;
    }
}

void coolbox_tree_classifier_free(void* handle) {
    delete static_cast<ClassifierHandle*>(handle);
}

int coolbox_tree_classifier_fit(void* handle,
                                const int* features,
                                size_t rows,
                                size_t columns,
                                const int* labels,
                                int max_depth,
                                char* error,
                                size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || labels == nullptr || max_depth < 0) {
            throw std::invalid_argument("classifier, labels, and nonnegative max_depth are required");
        }
        auto matrix = make_matrix(features, rows, columns);
        if (rows != 0 && rows > std::numeric_limits<size_t>::max() / sizeof(int)) {
            throw std::invalid_argument("target vector dimensions overflow");
        }
        std::vector<int> target(labels, labels + rows);
        auto& classifier = *static_cast<ClassifierHandle*>(handle);
        classifier.model.fit(matrix, target, max_depth);
        classifier.feature_count = columns;
        classifier.fitted = true;
    });
}

int coolbox_tree_classifier_predict(void* handle,
                                    const int* sample,
                                    size_t columns,
                                    int* prediction,
                                    char* error,
                                    size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || sample == nullptr || prediction == nullptr) {
            throw std::invalid_argument("classifier, sample, and prediction output are required");
        }
        auto& classifier = *static_cast<ClassifierHandle*>(handle);
        if (!classifier.fitted) {
            throw std::logic_error("decision tree classifier has not been fitted");
        }
        if (columns != classifier.feature_count) {
            throw std::invalid_argument("sample feature count does not match the fitted model");
        }
        *prediction = classifier.model.predict(std::vector<int>(sample, sample + columns));
    });
}

void* coolbox_tree_regressor_new() {
    try {
        return new RegressorHandle();
    } catch (...) {
        return nullptr;
    }
}

void coolbox_tree_regressor_free(void* handle) {
    delete static_cast<RegressorHandle*>(handle);
}

int coolbox_tree_regressor_fit(void* handle,
                               const double* features,
                               size_t rows,
                               size_t columns,
                               const double* targets,
                               int max_depth,
                               char* error,
                               size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || targets == nullptr) {
            throw std::invalid_argument("regressor and targets are required");
        }
        auto matrix = make_matrix(features, rows, columns);
        auto& regressor = *static_cast<RegressorHandle*>(handle);
        regressor.model.fit(matrix, std::vector<double>(targets, targets + rows), max_depth);
        regressor.feature_count = columns;
        regressor.fitted = true;
    });
}

int coolbox_tree_regressor_predict(void* handle,
                                   const double* sample,
                                   size_t columns,
                                   double* prediction,
                                   char* error,
                                   size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || sample == nullptr || prediction == nullptr) {
            throw std::invalid_argument("regressor, sample, and prediction output are required");
        }
        auto& regressor = *static_cast<RegressorHandle*>(handle);
        if (!regressor.fitted) {
            throw std::logic_error("decision tree regressor has not been fitted");
        }
        if (columns != regressor.feature_count) {
            throw std::invalid_argument("sample feature count does not match the fitted model");
        }
        *prediction = regressor.model.predict(std::vector<double>(sample, sample + columns));
    });
}

} // extern "C"
