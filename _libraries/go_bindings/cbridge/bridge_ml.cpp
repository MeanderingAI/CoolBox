#include "../abi/pca.h"
#include "../abi/svm.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../packages/ML/dimensionality_reduction/headers/pca.h"
#include "../../packages/ML/support_vector_machine/headers/linear_kernel.h"
#include "../../packages/ML/support_vector_machine/headers/rbf_kernel.h"
#include "../../packages/ML/support_vector_machine/headers/support_vector_machine.h"

#include "../../packages/ML/dimensionality_reduction/source/svd.cpp"
#include "../../packages/ML/dimensionality_reduction/source/pca.cpp"
#include "../../packages/ML/support_vector_machine/source/linear_kernel.cpp"
#include "../../packages/ML/support_vector_machine/source/rbf_kernel.cpp"
#include "../../packages/ML/support_vector_machine/source/support_vector_machine.cpp"

struct CoolBoxPCAModel {
    dimensionality_reduction::PCA impl;
    std::size_t feature_count = 0;

    CoolBoxPCAModel(int n_components, bool center, bool scale)
        : impl(n_components, center, scale) {}
};

struct CoolBoxSVMModel {
    std::unique_ptr<Kernel> kernel;
    std::unique_ptr<SVM> impl;
    std::size_t feature_count = 0;

    explicit CoolBoxSVMModel(std::unique_ptr<Kernel> kernel_impl)
        : kernel(std::move(kernel_impl)), impl(std::make_unique<SVM>(*kernel)) {}
};

namespace {

void clear_error(char** error_message) {
    if (error_message != nullptr) {
        *error_message = nullptr;
    }
}

void set_error(char** error_message, const std::string& message) {
    if (error_message == nullptr) {
        return;
    }

    char* buffer = static_cast<char*>(std::malloc(message.size() + 1));
    if (buffer == nullptr) {
        *error_message = nullptr;
        return;
    }

    std::memcpy(buffer, message.c_str(), message.size() + 1);
    *error_message = buffer;
}

template <typename Callback>
int run_bridge_call(char** error_message, Callback&& callback) {
    clear_error(error_message);
    try {
        callback();
        return 1;
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return 0;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return 0;
    }
}

mytrix::Matrix matrix_from_row_major(const double* flat_values, std::size_t rows, std::size_t cols) {
    if (flat_values == nullptr && rows * cols != 0) {
        throw std::invalid_argument("input matrix buffer is null");
    }

    mytrix::Matrix matrix(static_cast<mytrix::Index>(rows), static_cast<mytrix::Index>(cols));
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t col = 0; col < cols; ++col) {
            matrix(static_cast<mytrix::Index>(row), static_cast<mytrix::Index>(col)) = flat_values[row * cols + col];
        }
    }
    return matrix;
}

mytrix::Vector vector_from_buffer(const double* values, std::size_t count) {
    if (values == nullptr && count != 0) {
        throw std::invalid_argument("input vector buffer is null");
    }

    mytrix::Vector vector(static_cast<mytrix::Index>(count));
    for (std::size_t index = 0; index < count; ++index) {
        vector(static_cast<mytrix::Index>(index)) = values[index];
    }
    return vector;
}

void copy_matrix_to_row_major(const mytrix::Matrix& matrix, double* out_values, std::size_t out_count) {
    const std::size_t required = static_cast<std::size_t>(matrix.rows()) * static_cast<std::size_t>(matrix.cols());
    if (required == 0) {
        return;
    }
    if (out_values == nullptr) {
        throw std::invalid_argument("output matrix buffer is null");
    }
    if (out_count < required) {
        throw std::invalid_argument("output matrix buffer is too small");
    }

    for (mytrix::Index row = 0; row < matrix.rows(); ++row) {
        for (mytrix::Index col = 0; col < matrix.cols(); ++col) {
            out_values[static_cast<std::size_t>(row) * static_cast<std::size_t>(matrix.cols()) + static_cast<std::size_t>(col)] = matrix(row, col);
        }
    }
}

void copy_vector_to_buffer(const mytrix::Vector& vector, double* out_values, std::size_t out_count) {
    const std::size_t required = static_cast<std::size_t>(vector.size());
    if (required == 0) {
        return;
    }
    if (out_values == nullptr) {
        throw std::invalid_argument("output vector buffer is null");
    }
    if (out_count < required) {
        throw std::invalid_argument("output vector buffer is too small");
    }

    for (mytrix::Index index = 0; index < vector.size(); ++index) {
        out_values[static_cast<std::size_t>(index)] = vector(index);
    }
}

std::string normalize_kernel_name(const char* kernel_type) {
    std::string name = kernel_type != nullptr ? std::string(kernel_type) : std::string();
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return name;
}

std::unique_ptr<Kernel> create_kernel(const char* kernel_type, double param1, double /*param2*/, int /*param3*/) {
    const std::string normalized = normalize_kernel_name(kernel_type);
    if (normalized.empty() || normalized == "linear") {
        return std::make_unique<LinearKernel>();
    }
    if (normalized == "rbf") {
        return std::make_unique<RBFKernel>(param1);
    }

    throw std::invalid_argument("unsupported SVM kernel type: " + normalized);
}

}  // namespace

extern "C" {

CoolBoxPCAModel* coolbox_create_pca(int n_components, int center, int scale, char** error_message) {
    clear_error(error_message);
    try {
        return new CoolBoxPCAModel(n_components, center != 0, scale != 0);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

int coolbox_pca_fit(CoolBoxPCAModel* model, const double* x_flat, size_t rows, size_t cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        model->impl.fit(matrix_from_row_major(x_flat, rows, cols));
        model->feature_count = cols;
    });
}

int coolbox_pca_transform(const CoolBoxPCAModel* model, const double* x_flat, size_t rows, size_t cols, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        const Eigen::MatrixXd transformed = model->impl.transform(matrix_from_row_major(x_flat, rows, cols));
        copy_matrix_to_row_major(transformed, out_values, out_count);
    });
}

int coolbox_pca_fit_transform(CoolBoxPCAModel* model, const double* x_flat, size_t rows, size_t cols, double* out_values, size_t out_count, size_t* out_cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        const Eigen::MatrixXd transformed = model->impl.fit_transform(matrix_from_row_major(x_flat, rows, cols));
        model->feature_count = cols;
        if (out_cols != nullptr) {
            *out_cols = static_cast<size_t>(transformed.cols());
        }
        copy_matrix_to_row_major(transformed, out_values, out_count);
    });
}

int coolbox_pca_inverse_transform(const CoolBoxPCAModel* model, const double* x_flat, size_t rows, size_t cols, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        const Eigen::MatrixXd restored = model->impl.inverse_transform(matrix_from_row_major(x_flat, rows, cols));
        copy_matrix_to_row_major(restored, out_values, out_count);
    });
}

int coolbox_pca_get_components(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_matrix_to_row_major(model->impl.get_components(), out_values, out_count);
    });
}

int coolbox_pca_get_explained_variance(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_vector_to_buffer(model->impl.get_explained_variance(), out_values, out_count);
    });
}

int coolbox_pca_get_explained_variance_ratio(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_vector_to_buffer(model->impl.get_explained_variance_ratio(), out_values, out_count);
    });
}

int coolbox_pca_get_singular_values(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_vector_to_buffer(model->impl.get_singular_values(), out_values, out_count);
    });
}

int coolbox_pca_get_mean(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_vector_to_buffer(model->impl.get_mean(), out_values, out_count);
    });
}

int coolbox_pca_get_scale(const CoolBoxPCAModel* model, double* out_values, size_t out_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("pca model is null");
        }
        copy_vector_to_buffer(model->impl.get_scale(), out_values, out_count);
    });
}

size_t coolbox_pca_component_count(const CoolBoxPCAModel* model) {
    if (model == nullptr || !model->impl.is_fitted()) {
        return 0;
    }
    return static_cast<size_t>(model->impl.get_n_components());
}

size_t coolbox_pca_feature_count(const CoolBoxPCAModel* model) {
    if (model == nullptr) {
        return 0;
    }
    return model->feature_count;
}

int coolbox_pca_is_fitted(const CoolBoxPCAModel* model) {
    return (model != nullptr && model->impl.is_fitted()) ? 1 : 0;
}

void coolbox_free_pca(CoolBoxPCAModel* model) {
    delete model;
}

CoolBoxSVMModel* coolbox_create_svm(const char* kernel_type, double param1, double param2, int param3, char** error_message) {
    clear_error(error_message);
    try {
        return new CoolBoxSVMModel(create_kernel(kernel_type, param1, param2, param3));
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

int coolbox_svm_fit(CoolBoxSVMModel* model, const double* x_flat, size_t rows, size_t cols, const double* y, size_t y_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("svm model is null");
        }
        if (rows != y_count) {
            throw std::invalid_argument("target vector length must match the number of feature rows");
        }
        model->impl->fit(matrix_from_row_major(x_flat, rows, cols), vector_from_buffer(y, y_count));
        model->feature_count = cols;
    });
}

int coolbox_svm_predict(const CoolBoxSVMModel* model, const double* sample, size_t feature_count, double* out_value, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("svm model is null");
        }
        if (out_value == nullptr) {
            throw std::invalid_argument("output prediction buffer is null");
        }
        if (model->feature_count != 0 && feature_count != model->feature_count) {
            throw std::invalid_argument("sample feature count does not match fitted svm feature count");
        }
        *out_value = model->impl->predict(vector_from_buffer(sample, feature_count));
    });
}

void coolbox_free_svm(CoolBoxSVMModel* model) {
    delete model;
}

}  // extern "C"