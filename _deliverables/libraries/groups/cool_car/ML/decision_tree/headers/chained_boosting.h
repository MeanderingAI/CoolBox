#ifndef CHAINED_BOOSTING_H
#define CHAINED_BOOSTING_H

#include "lib_metadata.h"

#include <cstddef>
#include <vector>

struct ChainedBoostingParameters {
    unsigned int rounds = 100;
    double alpha_floor = 1e-9;
};

struct ChainedBoostingRule {
    std::size_t feature_index = 0;
    double threshold = 0.0;
    int polarity = 1;
    double alpha = 0.0;

    int evaluate(const std::vector<double>& sample) const;
};

class ChainedBoosting {
public:
    using StageFeatures = std::vector<std::vector<double>>;
    using TrainingFeatures = std::vector<StageFeatures>;
    using CostMatrix = std::vector<std::vector<double>>;

    explicit ChainedBoosting(const ChainedBoostingParameters& parameters = ChainedBoostingParameters());

    void fit(const TrainingFeatures& stage_features, const CostMatrix& costs);

    std::size_t predict_stop_stage(const StageFeatures& sample_stages) const;
    std::vector<double> decision_scores(const StageFeatures& sample_stages) const;
    double empirical_cost(const TrainingFeatures& stage_features, const CostMatrix& costs) const;

    std::size_t stage_count() const;
    std::size_t rules_per_stage(std::size_t stage_index) const;

private:
    ChainedBoostingParameters parameters_;
    std::vector<std::vector<ChainedBoostingRule>> stage_rules_;
};

#endif // CHAINED_BOOSTING_H