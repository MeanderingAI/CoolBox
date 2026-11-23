#include "indian_buffet_process.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ml {

IndianBuffetProcess::IndianBuffetProcess(double concentration)
    : IndianBuffetProcess(concentration, std::random_device{}()) {}

IndianBuffetProcess::IndianBuffetProcess(double concentration, std::uint32_t seed)
    : concentration_(concentration), generator_(seed) {
    if (!std::isfinite(concentration_) || concentration_ <= 0.0) {
        throw std::invalid_argument(
            "IndianBuffetProcess: concentration must be finite and positive");
    }
}

std::vector<std::size_t> IndianBuffetProcess::add_customer() {
    const std::size_t previous_customers = customer_features_.size();
    if (previous_customers == std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("IndianBuffetProcess: customer count overflow");
    }

    std::vector<std::size_t> selected_features;
    selected_features.reserve(feature_counts_.size());
    const double denominator = static_cast<double>(previous_customers) + concentration_;
    for (std::size_t feature = 0; feature < feature_counts_.size(); ++feature) {
        const double probability = static_cast<double>(feature_counts_[feature]) / denominator;
        if (std::bernoulli_distribution(probability)(generator_)) {
            selected_features.push_back(feature);
        }
    }

    const std::size_t customer_number = previous_customers + 1;
    const double new_feature_mean =
        concentration_ / static_cast<double>(customer_number);
    std::size_t new_feature_count = 0;
    if (new_feature_mean > 0.0) {
        new_feature_count =
            std::poisson_distribution<std::size_t>(new_feature_mean)(generator_);
    }

    if (new_feature_count > std::numeric_limits<std::size_t>::max() - feature_counts_.size()) {
        throw std::overflow_error("IndianBuffetProcess: feature count overflow");
    }
    const std::size_t total_features = feature_counts_.size() + new_feature_count;
    if (new_feature_count > selected_features.max_size() - selected_features.size()) {
        throw std::length_error("IndianBuffetProcess: too many new features");
    }

    selected_features.reserve(selected_features.size() + new_feature_count);
    customer_features_.reserve(previous_customers + 1);
    feature_counts_.reserve(total_features);

    for (std::size_t feature : selected_features) {
        ++feature_counts_[feature];
    }
    for (std::size_t feature = feature_counts_.size(); feature < total_features; ++feature) {
        selected_features.push_back(feature);
        feature_counts_.push_back(1);
    }

    customer_features_.push_back(std::move(selected_features));
    return customer_features_.back();
}

std::vector<std::vector<std::size_t>> IndianBuffetProcess::add_customers(std::size_t count) {
    std::vector<std::vector<std::size_t>> assignments;
    assignments.reserve(count);
    for (std::size_t customer = 0; customer < count; ++customer) {
        assignments.push_back(add_customer());
    }
    return assignments;
}

} // namespace ml
