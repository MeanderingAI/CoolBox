#include "random_forest.h"
#include <random>
#include <map>
#include <algorithm>

RandomForest::RandomForest(int num_trees, int max_depth)
    : num_trees_(num_trees), max_depth_(max_depth) {}

RandomForest::~RandomForest() {
    for (auto* tree : trees_) delete tree;
    trees_.clear();
}

void RandomForest::get_bootstrap_sample(
    const std::vector<std::vector<int>>& X_in,
    const std::vector<int>& y_in,
    std::vector<std::vector<int>>& X_out,
    std::vector<int>& y_out)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, X_in.size() - 1);

    size_t n = X_in.size();
    X_out.resize(n);
    y_out.resize(n);
    for (size_t i = 0; i < n; ++i) {
        size_t idx = dist(gen);
        X_out[i] = X_in[idx];
        y_out[i] = y_in[idx];
    }
}

void RandomForest::fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y) {
    for (auto* tree : trees_) delete tree;
    trees_.clear();

    for (int t = 0; t < num_trees_; ++t) {
        std::vector<std::vector<int>> X_boot;
        std::vector<int> y_boot;
        get_bootstrap_sample(X, y, X_boot, y_boot);

        auto* tree = new DecisionTree(SplitCriterion::GINI);
        tree->fit(X_boot, y_boot, max_depth_);
        trees_.push_back(tree);
    }
}

int RandomForest::predict(const std::vector<int>& sample) const {
    std::map<int, int> votes;
    for (const auto* tree : trees_)
        votes[tree->predict(sample)]++;

    int best_label = -1;
    int best_count = 0;
    for (const auto& [label, count] : votes) {
        if (count > best_count) {
            best_count = count;
            best_label = label;
        }
    }
    return best_label;
}
