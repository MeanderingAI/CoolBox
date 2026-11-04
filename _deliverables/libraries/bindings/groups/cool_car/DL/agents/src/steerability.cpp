#include "steerability.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace agents {
namespace {

void require_same_size(const GoalVector& a, const GoalVector& b, const char* context) {
    if (a.size() != b.size()) {
        throw std::invalid_argument(std::string(context) + ": goal vectors must have equal length");
    }
    if (a.empty()) {
        throw std::invalid_argument(std::string(context) + ": goal space must have a dimension");
    }
}

double norm(const GoalVector& v) {
    double sum = 0.0;
    for (double value : v) {
        sum += value * value;
    }
    return std::sqrt(sum);
}

} // namespace

double goal_distance(const GoalVector& a, const GoalVector& b) {
    require_same_size(a, b, "goal_distance");
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double difference = std::clamp(a[i], 0.0, 1.0) - std::clamp(b[i], 0.0, 1.0);
        sum += difference * difference;
    }
    return std::sqrt(sum);
}

SteeringDecomposition decompose(const SteeringSample& sample) {
    require_same_size(sample.source, sample.goal, "decompose");
    require_same_size(sample.source, sample.output, "decompose");

    const size_t dimensions = sample.source.size();
    GoalVector requested(dimensions);
    GoalVector realised(dimensions);
    for (size_t i = 0; i < dimensions; ++i) {
        requested[i] = sample.goal[i] - sample.source[i];
        realised[i] = sample.output[i] - sample.source[i];
    }

    SteeringDecomposition result;
    result.requested_distance = norm(requested);
    result.steering_error = goal_distance(sample.output, sample.goal);
    result.per_dimension_side_effect.assign(dimensions, 0.0);

    if (result.requested_distance <= 0.0) {
        // Nothing was asked for, so every bit of movement is a side effect.
        result.side_effect_magnitude = norm(realised);
        for (size_t i = 0; i < dimensions; ++i) {
            result.per_dimension_side_effect[i] = std::abs(realised[i]);
        }
        return result;
    }

    double projection = 0.0;
    for (size_t i = 0; i < dimensions; ++i) {
        projection += realised[i] * requested[i];
    }
    projection /= result.requested_distance;
    result.achieved_along_goal = projection;
    result.miscalibration = projection / result.requested_distance - 1.0;

    GoalVector orthogonal(dimensions);
    for (size_t i = 0; i < dimensions; ++i) {
        const double unit = requested[i] / result.requested_distance;
        orthogonal[i] = realised[i] - projection * unit;
        result.per_dimension_side_effect[i] = std::abs(orthogonal[i]);
    }
    result.side_effect_magnitude = norm(orthogonal);
    return result;
}

SteerabilityReport evaluate_steerability(const std::vector<SteeringSample>& samples) {
    SteerabilityReport report;
    if (samples.empty()) {
        return report;
    }

    const size_t dimensions = samples.front().source.size();
    report.samples = samples.size();
    report.mean_per_dimension_side_effect.assign(dimensions, 0.0);

    double baseline_error = 0.0;
    size_t undershoots = 0;
    for (const SteeringSample& sample : samples) {
        const SteeringDecomposition decomposition = decompose(sample);
        report.mean_steering_error += decomposition.steering_error;
        report.mean_miscalibration += decomposition.miscalibration;
        report.mean_side_effect += decomposition.side_effect_magnitude;
        if (decomposition.miscalibration < 0.0) {
            ++undershoots;
        }
        for (size_t i = 0; i < dimensions; ++i) {
            report.mean_per_dimension_side_effect[i] += decomposition.per_dimension_side_effect[i];
        }
        baseline_error += goal_distance(sample.source, sample.goal);
    }

    const double count = static_cast<double>(samples.size());
    report.mean_steering_error /= count;
    report.mean_miscalibration /= count;
    report.mean_side_effect /= count;
    report.undershoot_rate = static_cast<double>(undershoots) / count;
    for (double& value : report.mean_per_dimension_side_effect) {
        value /= count;
    }

    baseline_error /= count;
    report.steerability_index =
        baseline_error > 0.0 ? 1.0 - report.mean_steering_error / baseline_error : 0.0;
    return report;
}

size_t worst_side_effect_dimension(const SteerabilityReport& report) {
    if (report.mean_per_dimension_side_effect.empty()) {
        throw std::invalid_argument("worst_side_effect_dimension: report has no dimensions");
    }
    const auto worst = std::max_element(report.mean_per_dimension_side_effect.begin(),
                                        report.mean_per_dimension_side_effect.end());
    return static_cast<size_t>(std::distance(report.mean_per_dimension_side_effect.begin(), worst));
}

std::vector<GoalVector> sample_goals(const GoalVector& source,
                                     size_t count,
                                     double min_distance,
                                     unsigned int seed) {
    if (source.empty()) {
        throw std::invalid_argument("sample_goals: goal space must have a dimension");
    }
    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    std::vector<GoalVector> goals;
    goals.reserve(count);
    // Rejection sampling keeps every probe a real request for change.
    const size_t attempt_limit = 1000 * (count + 1);
    for (size_t attempt = 0; attempt < attempt_limit && goals.size() < count; ++attempt) {
        GoalVector candidate(source.size());
        for (double& value : candidate) {
            value = distribution(generator);
        }
        if (goal_distance(candidate, source) >= min_distance) {
            goals.push_back(std::move(candidate));
        }
    }
    if (goals.size() < count) {
        throw std::runtime_error("sample_goals: min_distance is too large for this goal space");
    }
    return goals;
}

size_t best_of_n(const std::vector<GoalVector>& candidates, const GoalVector& goal) {
    if (candidates.empty()) {
        throw std::invalid_argument("best_of_n: no candidates");
    }
    size_t best = 0;
    double best_distance = goal_distance(candidates[0], goal);
    for (size_t i = 1; i < candidates.size(); ++i) {
        const double distance = goal_distance(candidates[i], goal);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

GoalVector SteeringSimulator::respond(const GoalVector& source, const GoalVector& goal) const {
    require_same_size(source, goal, "SteeringSimulator::respond");
    if (side_effect_dimension >= source.size()) {
        throw std::out_of_range("SteeringSimulator: side_effect_dimension is out of range");
    }

    const size_t dimensions = source.size();
    GoalVector output(dimensions);
    double travelled = 0.0;
    for (size_t i = 0; i < dimensions; ++i) {
        const double step = gain * (goal[i] - source[i]);
        travelled += step * step;
        output[i] = source[i] + step;
    }

    output[side_effect_dimension] += side_effect_strength * std::sqrt(travelled);
    for (double& value : output) {
        value = std::clamp(value, 0.0, 1.0);
    }
    return output;
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
