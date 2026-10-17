#include "random_forest_regressor.h"
#include "tree_ensemble_utils.h"

#include <utility>

RandomForestRegressor::RandomForestRegressor(int num_trees, int max_depth, unsigned int seed)
    : num_trees_(num_trees), max_depth_(max_depth), seed_(seed) {
    if (num_trees <= 0 || max_depth < 0) {
        throw std::invalid_argument("Forest requires positive num_trees and nonnegative max_depth");
    }
}

void RandomForestRegressor::fit(const std::vector<std::vector<double>>& X,
                                const std::vector<double>& y) {
    const auto features = tree_ensemble_detail::validate_training(X, y);
    std::mt19937 generator(seed_);
    std::vector<std::unique_ptr<DecisionTreeRegressor>> trees;
    trees.reserve(num_trees_);
    for (int i = 0; i < num_trees_; ++i) {
        std::vector<std::vector<double>> sampled_X;
        std::vector<double> sampled_y;
        tree_ensemble_detail::bootstrap(X, y, sampled_X, sampled_y, generator);
        auto tree = std::make_unique<DecisionTreeRegressor>();
        tree->fit(sampled_X, sampled_y, max_depth_);
        trees.push_back(std::move(tree));
    }
    trees_ = std::move(trees);
    num_features_ = features;
}

double RandomForestRegressor::predict(const std::vector<double>& sample) const {
    tree_ensemble_detail::validate_sample(sample, num_features_);
    long double mean = 0.0L;
    std::size_t count = 0;
    for (const auto& tree : trees_) {
        mean += (static_cast<long double>(tree->predict(sample)) - mean) / ++count;
    }
    return tree_ensemble_detail::checked_value(mean);
}
