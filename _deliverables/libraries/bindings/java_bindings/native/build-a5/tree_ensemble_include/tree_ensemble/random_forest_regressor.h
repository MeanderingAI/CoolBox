#ifndef RANDOM_FOREST_REGRESSOR_H
#define RANDOM_FOREST_REGRESSOR_H

#include "decision_tree_regressor.h"

/**
 * @brief Bootstrapped regression trees aggregated by their mean prediction.
 *
 * Like RandomForest, this uses all features in each tree (bagging, without
 * per-split feature subsampling). A fixed seed makes fits reproducible.
 * Invalid parameters/data throw std::invalid_argument; prediction before fit
 * throws std::logic_error. Failed fits preserve the previous model.
 */
class RandomForestRegressor {
public:
    RandomForestRegressor(int num_trees, int max_depth, unsigned int seed = 5489u);
    RandomForestRegressor(const RandomForestRegressor&) = delete;
    RandomForestRegressor& operator=(const RandomForestRegressor&) = delete;
    void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y);
    double predict(const std::vector<double>& sample) const;

private:
    int num_trees_;
    int max_depth_;
    unsigned int seed_;
    std::size_t num_features_ = 0;
    std::vector<std::unique_ptr<DecisionTreeRegressor>> trees_;
};

#endif
