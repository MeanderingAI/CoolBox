#include "../include/synthetic_data_bindings.hpp"

#include <pybind11/numpy.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require_positive(const char* name, py::ssize_t value) {
    if (value <= 0)
        throw py::value_error(std::string(name) + " must be positive");
}

py::tuple make_regression(
    py::ssize_t n_samples,
    py::ssize_t n_features,
    double noise,
    double bias,
    std::uint64_t seed) {
    require_positive("n_samples", n_samples);
    require_positive("n_features", n_features);
    if (noise < 0.0)
        throw py::value_error("noise must be non-negative");

    py::array_t<double> features({n_samples, n_features});
    py::array_t<double> targets(n_samples);
    auto feature_view = features.mutable_unchecked<2>();
    auto target_view = targets.mutable_unchecked<1>();

    std::mt19937_64 engine(seed);
    std::normal_distribution<double> feature_distribution(0.0, 1.0);
    std::normal_distribution<double> noise_distribution(
        0.0, noise == 0.0 ? 1.0 : noise);
    std::uniform_real_distribution<double> coefficient_distribution(-10.0, 10.0);
    std::vector<double> coefficients(static_cast<std::size_t>(n_features));
    for (double& coefficient : coefficients)
        coefficient = coefficient_distribution(engine);

    for (py::ssize_t row = 0; row < n_samples; ++row) {
        double target = bias;
        for (py::ssize_t column = 0; column < n_features; ++column) {
            const double value = feature_distribution(engine);
            feature_view(row, column) = value;
            target += value * coefficients[static_cast<std::size_t>(column)];
        }
        target_view(row) = target + (noise == 0.0 ? 0.0 : noise_distribution(engine));
    }

    return py::make_tuple(std::move(features), std::move(targets));
}

py::tuple make_classification(
    py::ssize_t n_samples,
    py::ssize_t n_features,
    py::ssize_t n_classes,
    double class_sep,
    std::uint64_t seed) {
    require_positive("n_samples", n_samples);
    require_positive("n_features", n_features);
    if (n_classes < 2)
        throw py::value_error("n_classes must be at least 2");
    if (n_classes > n_samples)
        throw py::value_error("n_classes cannot exceed n_samples");
    if (class_sep <= 0.0)
        throw py::value_error("class_sep must be positive");

    py::array_t<double> features({n_samples, n_features});
    py::array_t<std::int64_t> labels(n_samples);
    auto feature_view = features.mutable_unchecked<2>();
    auto label_view = labels.mutable_unchecked<1>();

    std::mt19937_64 engine(seed);
    std::normal_distribution<double> sample_distribution(0.0, 1.0);
    std::uniform_real_distribution<double> center_distribution(-class_sep, class_sep);
    std::vector<double> centers(
        static_cast<std::size_t>(n_classes * n_features));
    for (double& center : centers)
        center = center_distribution(engine);
    for (py::ssize_t class_index = 0; class_index < n_classes; ++class_index) {
        centers[static_cast<std::size_t>(class_index * n_features)] =
            class_sep * (2.0 * class_index / (n_classes - 1) - 1.0);
    }

    std::vector<std::int64_t> shuffled_labels(static_cast<std::size_t>(n_samples));
    for (py::ssize_t row = 0; row < n_samples; ++row)
        shuffled_labels[static_cast<std::size_t>(row)] = row % n_classes;
    std::shuffle(shuffled_labels.begin(), shuffled_labels.end(), engine);

    for (py::ssize_t row = 0; row < n_samples; ++row) {
        const std::int64_t label = shuffled_labels[static_cast<std::size_t>(row)];
        label_view(row) = label;
        for (py::ssize_t column = 0; column < n_features; ++column) {
            const std::size_t center_index =
                static_cast<std::size_t>(label * n_features + column);
            feature_view(row, column) = centers[center_index] + sample_distribution(engine);
        }
    }

    return py::make_tuple(std::move(features), std::move(labels));
}

py::tuple make_blobs(
    py::ssize_t n_samples,
    py::ssize_t centers_count,
    py::ssize_t n_features,
    double cluster_std,
    std::uint64_t seed) {
    require_positive("n_samples", n_samples);
    require_positive("centers", centers_count);
    require_positive("n_features", n_features);
    if (centers_count > n_samples)
        throw py::value_error("centers cannot exceed n_samples");
    if (cluster_std < 0.0)
        throw py::value_error("cluster_std must be non-negative");

    py::array_t<double> features({n_samples, n_features});
    py::array_t<std::int64_t> labels(n_samples);
    auto feature_view = features.mutable_unchecked<2>();
    auto label_view = labels.mutable_unchecked<1>();

    std::mt19937_64 engine(seed);
    std::uniform_real_distribution<double> center_distribution(-10.0, 10.0);
    std::normal_distribution<double> offset_distribution(
        0.0, cluster_std == 0.0 ? 1.0 : cluster_std);
    std::vector<double> center_values(
        static_cast<std::size_t>(centers_count * n_features));
    for (double& center : center_values)
        center = center_distribution(engine);

    std::vector<std::int64_t> shuffled_labels(static_cast<std::size_t>(n_samples));
    for (py::ssize_t row = 0; row < n_samples; ++row)
        shuffled_labels[static_cast<std::size_t>(row)] = row % centers_count;
    std::shuffle(shuffled_labels.begin(), shuffled_labels.end(), engine);

    for (py::ssize_t row = 0; row < n_samples; ++row) {
        const std::int64_t label = shuffled_labels[static_cast<std::size_t>(row)];
        label_view(row) = label;
        for (py::ssize_t column = 0; column < n_features; ++column) {
            const std::size_t center_index =
                static_cast<std::size_t>(label * n_features + column);
            feature_view(row, column) =
                center_values[center_index] +
                (cluster_std == 0.0 ? 0.0 : offset_distribution(engine));
        }
    }

    return py::make_tuple(std::move(features), std::move(labels));
}

} // namespace

void bind_synthetic_data(py::module_& parent_module) {
    py::module_ module = parent_module.def_submodule(
        "synthetic_data", "Deterministic native synthetic dataset generators.");

    module.def(
        "make_regression",
        &make_regression,
        py::arg("n_samples") = 100,
        py::arg("n_features") = 1,
        py::arg("noise") = 0.0,
        py::arg("bias") = 0.0,
        py::arg("seed") = 0,
        "Generate a regression feature matrix and continuous target vector.");
    module.def(
        "make_classification",
        &make_classification,
        py::arg("n_samples") = 100,
        py::arg("n_features") = 2,
        py::arg("n_classes") = 2,
        py::arg("class_sep") = 2.0,
        py::arg("seed") = 0,
        "Generate a balanced classification feature matrix and integer labels.");
    module.def(
        "make_blobs",
        &make_blobs,
        py::arg("n_samples") = 100,
        py::arg("centers") = 3,
        py::arg("n_features") = 2,
        py::arg("cluster_std") = 1.0,
        py::arg("seed") = 0,
        "Generate isotropic Gaussian blobs and integer cluster labels.");
}
