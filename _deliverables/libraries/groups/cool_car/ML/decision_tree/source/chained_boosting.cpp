#include "chained_boosting.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {

constexpr double weight_floor = 1e-12;

struct CandidateRule {
    ChainedBoostingRule rule;
    double score = -std::numeric_limits<double>::infinity();
    double alpha = 0.0;
};

std::size_t infer_stage_count(const ChainedBoosting::TrainingFeatures& stage_features, const ChainedBoosting::CostMatrix& costs) {
    if (stage_features.empty() || costs.empty() || stage_features.size() != costs.size()) {
        throw std::invalid_argument("ChainedBoosting requires aligned features and costs");
    }
    if (stage_features.front().empty() || costs.front().size() != stage_features.front().size() + 1) {
        throw std::invalid_argument("ChainedBoosting costs must contain one entry per stop stage plus one pass-through cost");
    }
    const std::size_t stage_count = stage_features.front().size();
    std::vector<std::size_t> feature_dimensions(stage_count, 0);
    for (std::size_t stage_index = 0; stage_index < stage_count; ++stage_index) {
        if (stage_features.front()[stage_index].empty()) {
            throw std::invalid_argument("ChainedBoosting stages must expose at least one feature");
        }
        feature_dimensions[stage_index] = stage_features.front()[stage_index].size();
    }

    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        if (stage_features[sample_index].size() != stage_count || costs[sample_index].size() != stage_count + 1) {
            throw std::invalid_argument("ChainedBoosting samples must share stage and cost dimensions");
        }
        for (std::size_t cost_index = 0; cost_index < costs[sample_index].size(); ++cost_index) {
            if (costs[sample_index][cost_index] < 0.0) {
                throw std::invalid_argument("ChainedBoosting expects non-negative shifted costs");
            }
        }
        for (std::size_t stage_index = 0; stage_index < stage_count; ++stage_index) {
            if (stage_features[sample_index][stage_index].size() != feature_dimensions[stage_index]) {
                throw std::invalid_argument("ChainedBoosting stage feature dimensions must be consistent");
            }
        }
    }
    return stage_count;
}

double future_weight_sum(const std::vector<std::vector<double>>& weights, std::size_t first_stage, std::size_t sample_index) {
    double sum = 0.0;
    for (std::size_t stage_index = first_stage; stage_index < weights.size(); ++stage_index) {
        sum += weights[stage_index][sample_index];
    }
    return sum;
}

double signed_rule_score(
    const ChainedBoosting::TrainingFeatures& stage_features,
    const std::vector<double>& targets,
    std::size_t stage_index,
    const ChainedBoostingRule& rule) {
    double score = 0.0;
    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        score += targets[sample_index] * static_cast<double>(rule.evaluate(stage_features[sample_index][stage_index]));
    }
    return score;
}

ChainedBoostingRule fit_weighted_stump(
    const ChainedBoosting::TrainingFeatures& stage_features,
    const std::vector<double>& targets,
    std::size_t stage_index) {
    ChainedBoostingRule best_rule;
    double best_score = -std::numeric_limits<double>::infinity();
    const std::size_t feature_count = stage_features.front()[stage_index].size();

    for (int polarity : {1, -1}) {
        ChainedBoostingRule constant_rule;
        constant_rule.polarity = polarity;
        constant_rule.threshold = std::numeric_limits<double>::infinity();
        const double score = signed_rule_score(stage_features, targets, stage_index, constant_rule);
        if (score > best_score) {
            best_score = score;
            best_rule = constant_rule;
        }
    }

    for (std::size_t feature_index = 0; feature_index < feature_count; ++feature_index) {
        std::vector<double> values;
        values.reserve(stage_features.size());
        for (const auto& sample_stages : stage_features) {
            values.push_back(sample_stages[stage_index][feature_index]);
        }
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());

        std::vector<double> thresholds;
        thresholds.reserve(values.size() + 1);
        thresholds.push_back(values.front() - 1.0);
        for (std::size_t value_index = 0; value_index + 1 < values.size(); ++value_index) {
            thresholds.push_back((values[value_index] + values[value_index + 1]) / 2.0);
        }
        thresholds.push_back(values.back() + 1.0);

        for (double threshold : thresholds) {
            for (int polarity : {1, -1}) {
                ChainedBoostingRule rule;
                rule.feature_index = feature_index;
                rule.threshold = threshold;
                rule.polarity = polarity;
                const double score = signed_rule_score(stage_features, targets, stage_index, rule);
                if (score > best_score) {
                    best_score = score;
                    best_rule = rule;
                }
            }
        }
    }
    return best_rule;
}

CandidateRule evaluate_candidate(
    const ChainedBoosting::TrainingFeatures& stage_features,
    const std::vector<std::vector<double>>& weights,
    std::size_t stage_index,
    const ChainedBoostingRule& rule) {
    double emphasize_weight = 0.0;
    double skipped_weight = 0.0;

    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        const int prediction = rule.evaluate(stage_features[sample_index][stage_index]);
        const double stop_weight = weights[stage_index][sample_index];
        const double future_weight = future_weight_sum(weights, stage_index + 1, sample_index);
        if (prediction > 0) {
            emphasize_weight += stop_weight;
            skipped_weight += future_weight;
        } else {
            emphasize_weight += future_weight;
            skipped_weight += stop_weight;
        }
    }

    CandidateRule candidate;
    candidate.rule = rule;
    candidate.alpha = 0.5 * std::log(std::max(skipped_weight, weight_floor) / std::max(emphasize_weight, weight_floor));
    candidate.score = emphasize_weight * (1.0 - std::exp(candidate.alpha))
        + skipped_weight * (1.0 - std::exp(-candidate.alpha));
    candidate.rule.alpha = candidate.alpha;
    return candidate;
}

} // namespace

int ChainedBoostingRule::evaluate(const std::vector<double>& sample) const {
    const int base_prediction = sample[feature_index] <= threshold ? 1 : -1;
    return polarity * base_prediction;
}

ChainedBoosting::ChainedBoosting(const ChainedBoostingParameters& parameters)
    : parameters_(parameters) {}

void ChainedBoosting::fit(const TrainingFeatures& stage_features, const CostMatrix& costs) {
    const std::size_t stage_count_value = infer_stage_count(stage_features, costs);
    const std::size_t sample_count = stage_features.size();
    stage_rules_.assign(stage_count_value, {});

    std::vector<std::vector<double>> weights(stage_count_value + 1, std::vector<double>(sample_count, 0.0));
    for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
        for (std::size_t stage_index = 0; stage_index <= stage_count_value; ++stage_index) {
            weights[stage_index][sample_index] = costs[sample_index][stage_index];
        }
    }

    for (unsigned int round_index = 0; round_index < parameters_.rounds; ++round_index) {
        CandidateRule best_candidate;
        std::size_t best_stage = stage_count_value;

        for (std::size_t stage_index = 0; stage_index < stage_count_value; ++stage_index) {
            std::vector<double> targets(sample_count, 0.0);
            for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
                targets[sample_index] = future_weight_sum(weights, stage_index + 1, sample_index)
                    - weights[stage_index][sample_index];
            }

            const ChainedBoostingRule rule = fit_weighted_stump(stage_features, targets, stage_index);
            const CandidateRule candidate = evaluate_candidate(stage_features, weights, stage_index, rule);
            if (candidate.score > best_candidate.score) {
                best_candidate = candidate;
                best_stage = stage_index;
            }
        }

        if (best_stage == stage_count_value || best_candidate.alpha <= parameters_.alpha_floor) {
            break;
        }

        stage_rules_[best_stage].push_back(best_candidate.rule);
        for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
            const int prediction = best_candidate.rule.evaluate(stage_features[sample_index][best_stage]);
            weights[best_stage][sample_index] *= std::exp(best_candidate.alpha * static_cast<double>(prediction));
            for (std::size_t later_stage = best_stage + 1; later_stage <= stage_count_value; ++later_stage) {
                weights[later_stage][sample_index] *= std::exp(-best_candidate.alpha * static_cast<double>(prediction));
            }
        }
    }
}

std::size_t ChainedBoosting::predict_stop_stage(const StageFeatures& sample_stages) const {
    if (sample_stages.size() != stage_rules_.size()) {
        throw std::invalid_argument("ChainedBoosting prediction sample has the wrong number of stages");
    }
    for (std::size_t stage_index = 0; stage_index < stage_rules_.size(); ++stage_index) {
        double score = 0.0;
        for (const auto& rule : stage_rules_[stage_index]) {
            score += rule.alpha * static_cast<double>(rule.evaluate(sample_stages[stage_index]));
        }
        if (score > 0.0) return stage_index;
    }
    return stage_rules_.size();
}

std::vector<double> ChainedBoosting::decision_scores(const StageFeatures& sample_stages) const {
    if (sample_stages.size() != stage_rules_.size()) {
        throw std::invalid_argument("ChainedBoosting score sample has the wrong number of stages");
    }
    std::vector<double> scores(stage_rules_.size(), 0.0);
    for (std::size_t stage_index = 0; stage_index < stage_rules_.size(); ++stage_index) {
        for (const auto& rule : stage_rules_[stage_index]) {
            scores[stage_index] += rule.alpha * static_cast<double>(rule.evaluate(sample_stages[stage_index]));
        }
    }
    return scores;
}

double ChainedBoosting::empirical_cost(const TrainingFeatures& stage_features, const CostMatrix& costs) const {
    if (stage_features.empty() || stage_features.size() != costs.size()) {
        throw std::invalid_argument("ChainedBoosting empirical cost requires aligned features and costs");
    }
    double total_cost = 0.0;
    for (std::size_t sample_index = 0; sample_index < stage_features.size(); ++sample_index) {
        total_cost += costs[sample_index][predict_stop_stage(stage_features[sample_index])];
    }
    return total_cost / static_cast<double>(stage_features.size());
}

std::size_t ChainedBoosting::stage_count() const {
    return stage_rules_.size();
}

std::size_t ChainedBoosting::rules_per_stage(std::size_t stage_index) const {
    if (stage_index >= stage_rules_.size()) {
        throw std::out_of_range("ChainedBoosting stage index out of range");
    }
    return stage_rules_[stage_index].size();
}