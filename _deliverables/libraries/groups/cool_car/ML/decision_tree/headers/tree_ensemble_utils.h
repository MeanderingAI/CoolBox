#ifndef TREE_ENSEMBLE_UTILS_H
#define TREE_ENSEMBLE_UTILS_H

#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

namespace tree_ensemble_detail {

template <typename Target>
std::size_t validate_training(const std::vector<std::vector<double>>& X,
                              const std::vector<Target>& y) {
    if (X.empty() || X.size() != y.size() || X.front().empty()) {
        throw std::invalid_argument("Training requires nonempty matching data and features");
    }
    const auto features = X.front().size();
    for (std::size_t i = 0; i < X.size(); ++i) {
        if (X[i].size() != features || !std::isfinite(y[i])) {
            throw std::invalid_argument("Training requires rectangular features and finite targets");
        }
        for (double value : X[i]) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("Training features must be finite");
            }
        }
    }
    return features;
}

inline void validate_sample(const std::vector<double>& sample, std::size_t features) {
    if (features == 0) {
        throw std::logic_error("Model must be fitted before prediction");
    }
    if (sample.size() != features) {
        throw std::invalid_argument("Sample feature count must match training");
    }
    for (double value : sample) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Sample features must be finite");
        }
    }
}

inline double checked_value(long double value) {
    const double result = static_cast<double>(value);
    if (!std::isfinite(result)) {
        throw std::overflow_error("Ensemble prediction exceeds the numeric range");
    }
    return result;
}

template <typename Feature, typename Target>
void bootstrap(const std::vector<std::vector<Feature>>& X, const std::vector<Target>& y,
               std::vector<std::vector<Feature>>& sampled_X, std::vector<Target>& sampled_y,
               std::mt19937& generator) {
    std::uniform_int_distribution<std::size_t> distribution(0, X.size() - 1);
    sampled_X.resize(X.size());
    sampled_y.resize(y.size());
    for (std::size_t i = 0; i < X.size(); ++i) {
        const auto index = distribution(generator);
        sampled_X[i] = X[index];
        sampled_y[i] = y[index];
    }
}

} // namespace tree_ensemble_detail

#endif
