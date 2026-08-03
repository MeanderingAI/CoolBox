#include "catenary_support_vector_machine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

std::size_t infer_stage_count(
    const CatenarySupportVectorMachine::TrainingFeatures& stage_features,
    const CatenarySupportVectorMachine::CostMatrix& costs) {
    if (stage_features.empty() || costs.empty() || stage_features.size() != costs.size()) {
        throw std::invalid_argument("catSVM requires aligned features and costs");
    }
    if (stage_features.front().empty() || costs.front().size() != stage_features.front().size() + 1) {
        throw std::invalid_argument("catSVM costs must contain one stop cost per stage plus one pass-through cost");
    }

    const std::size_t stage_count = stage_features.front().size();
    std::vector<std::size_t> dimensions(stage_count, 0);
    for (std::size_t stage_index = 0; stage_index < stage_count; ++stage_index) {
        if (stage_features.front()[stage_index].empty()) {
            throw std::invalid_argument("catSVM stages must expose at least one feature");
        }
        dimensions[stage_index] = stage_features.front()[stage_index].size();
    }

    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        if (stage_features[sample_index].size() != stage_count || costs[sample_index].size() != stage_count + 1) {
            throw std::invalid_argument("catSVM samples must share stage and cost dimensions");
        }
        for (double cost : costs[sample_index]) {
            if (cost < 0.0) throw std::invalid_argument("catSVM expects non-negative shifted costs");
        }
        for (std::size_t stage_index = 0; stage_index < stage_count; ++stage_index) {
            if (stage_features[sample_index][stage_index].size() != dimensions[stage_index]) {
                throw std::invalid_argument("catSVM stage feature dimensions must be consistent");
            }
        }
    }

    return stage_count;
}

double min_future_cost(const std::vector<double>& costs, std::size_t stage_index) {
    double value = std::numeric_limits<double>::infinity();
    for (std::size_t cost_index = stage_index + 1; cost_index < costs.size(); ++cost_index) {
        value = std::min(value, costs[cost_index]);
    }
    return value;
}

double squared_norm(const std::vector<double>& values) {
    double total = 0.0;
    for (double value : values) total += value * value;
    return total;
}

} // namespace

double CatenarySVMStageModel::score(const std::vector<double>& sample) const {
    if (sample.size() != weights.size()) {
        throw std::invalid_argument("catSVM sample dimension does not match stage model");
    }
    double result = bias;
    for (std::size_t index = 0; index < sample.size(); ++index) {
        result += weights[index] * sample[index];
    }
    return result;
}

CatenarySupportVectorMachine::CatenarySupportVectorMachine(const CatenarySVMParameters& parameters)
    : parameters_(parameters) {}

void CatenarySupportVectorMachine::fit(const TrainingFeatures& stage_features, const CostMatrix& costs) {
    const std::size_t stage_count_value = infer_stage_count(stage_features, costs);
    stages_.assign(stage_count_value, {});
    for (std::size_t stage_index = 0; stage_index < stage_count_value; ++stage_index) {
        stages_[stage_index].weights.assign(stage_features.front()[stage_index].size(), 0.0);
        stages_[stage_index].bias = 0.0;
    }

    for (unsigned int epoch = 0; epoch < parameters_.epochs; ++epoch) {
        for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
            bool reaches_stage = true;
            for (std::size_t stage_index = 0; stage_index < stage_count_value && reaches_stage; ++stage_index) {
                const double stop_cost = costs[sample_index][stage_index];
                const double future_cost = min_future_cost(costs[sample_index], stage_index);
                const double label = future_cost + parameters_.reach_margin < stop_cost ? 1.0 : -1.0;
                const double importance = std::max(std::abs(stop_cost - future_cost), 1e-6);
                CatenarySVMStageModel& stage = stages_[stage_index];

                for (double& weight : stage.weights) {
                    weight -= parameters_.learning_rate * parameters_.regularization * weight;
                }

                const double margin = label * stage.score(stage_features[sample_index][stage_index]);
                if (margin < 1.0) {
                    const auto& features = stage_features[sample_index][stage_index];
                    for (std::size_t feature_index = 0; feature_index < features.size(); ++feature_index) {
                        stage.weights[feature_index] += parameters_.learning_rate * importance * label * features[feature_index];
                    }
                    stage.bias += parameters_.learning_rate * importance * label;
                }

                reaches_stage = stage.score(stage_features[sample_index][stage_index]) >= 0.0;
            }
        }
    }

    for (auto& stage : stages_) {
        const double norm = std::sqrt(squared_norm(stage.weights));
        if (norm > 1e6) {
            for (double& weight : stage.weights) weight /= norm;
            stage.bias /= norm;
        }
    }
}

std::size_t CatenarySupportVectorMachine::predict_stop_stage(const StageFeatures& sample_stages) const {
    if (sample_stages.size() != stages_.size()) {
        throw std::invalid_argument("catSVM prediction sample has the wrong number of stages");
    }
    for (std::size_t stage_index = 0; stage_index < stages_.size(); ++stage_index) {
        if (stages_[stage_index].score(sample_stages[stage_index]) < 0.0) return stage_index;
    }
    return stages_.size();
}

std::vector<double> CatenarySupportVectorMachine::decision_scores(const StageFeatures& sample_stages) const {
    if (sample_stages.size() != stages_.size()) {
        throw std::invalid_argument("catSVM score sample has the wrong number of stages");
    }
    std::vector<double> scores;
    scores.reserve(stages_.size());
    for (std::size_t stage_index = 0; stage_index < stages_.size(); ++stage_index) {
        scores.push_back(stages_[stage_index].score(sample_stages[stage_index]));
    }
    return scores;
}

double CatenarySupportVectorMachine::empirical_cost(const TrainingFeatures& stage_features, const CostMatrix& costs) const {
    if (stage_features.empty() || stage_features.size() != costs.size()) {
        throw std::invalid_argument("catSVM empirical cost requires aligned features and costs");
    }
    double total_cost = 0.0;
    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        total_cost += costs[sample_index][predict_stop_stage(stage_features[sample_index])];
    }
    return total_cost / static_cast<double>(stage_features.size());
}

std::size_t CatenarySupportVectorMachine::stage_count() const {
    return stages_.size();
}

const CatenarySVMStageModel& CatenarySupportVectorMachine::stage_model(std::size_t stage_index) const {
    if (stage_index >= stages_.size()) {
        throw std::out_of_range("catSVM stage index out of range");
    }
    return stages_[stage_index];
}