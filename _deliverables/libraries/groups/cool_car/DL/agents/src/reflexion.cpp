#include "reflexion.h"

#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace agents {

ReflexionAgent::ReflexionAgent(ReflexionActor actor,
                               ReflexionEvaluator evaluator,
                               ReflexionSelfReflection self_reflection)
    : ReflexionAgent(std::move(actor),
                     std::move(evaluator),
                     std::move(self_reflection),
                     Config{}) {}

ReflexionAgent::ReflexionAgent(ReflexionActor actor,
                               ReflexionEvaluator evaluator,
                               ReflexionSelfReflection self_reflection,
                               Config config)
    : actor_(std::move(actor)),
      evaluator_(std::move(evaluator)),
      self_reflection_(std::move(self_reflection)),
      config_(config) {
    if (!actor_ || !evaluator_ || !self_reflection_) {
        throw std::invalid_argument("ReflexionAgent: all three model callbacks are required");
    }
    if (config_.max_trials == 0) {
        throw std::invalid_argument("ReflexionAgent: max_trials must be positive");
    }
    if (config_.memory_capacity == 0) {
        throw std::invalid_argument("ReflexionAgent: memory_capacity must be positive");
    }
}

void ReflexionAgent::remember(std::string reflection) {
    if (reflection.empty()) {
        throw std::runtime_error("ReflexionAgent: self-reflection must not be empty");
    }
    if (memory_.size() == config_.memory_capacity) {
        memory_.erase(memory_.begin());
    }
    memory_.push_back(std::move(reflection));
}

ReflexionResult ReflexionAgent::run(const std::string& task) {
    ReflexionResult result;
    result.trials.reserve(config_.max_trials);

    for (size_t trial_index = 0; trial_index < config_.max_trials; ++trial_index) {
        ReflexionTrial trial;
        trial.trajectory = actor_(task, memory_);
        trial.evaluation = evaluator_(task, trial.trajectory);

        if (trial.evaluation.passed) {
            result.passed = true;
            result.trials.push_back(std::move(trial));
            break;
        }

        trial.reflection =
            self_reflection_(task, trial.trajectory, trial.evaluation, memory_);
        remember(trial.reflection);
        result.trials.push_back(std::move(trial));
    }

    result.memory = memory_;
    return result;
}

ReflexionFailureCause detect_alfworld_failure(
    const ReflexionTrajectory& trajectory,
    size_t repeated_cycle_limit,
    size_t action_limit) {
    if (trajectory.steps.size() > action_limit) {
        return ReflexionFailureCause::action_limit;
    }
    if (repeated_cycle_limit == 0 || trajectory.steps.empty()) {
        return ReflexionFailureCause::none;
    }

    size_t run_length = 1;
    for (size_t i = 1; i < trajectory.steps.size(); ++i) {
        const ReflexionStep& current = trajectory.steps[i];
        const ReflexionStep& previous = trajectory.steps[i - 1];
        if (current.action == previous.action &&
            current.observation == previous.observation) {
            ++run_length;
            if (run_length > repeated_cycle_limit) {
                return ReflexionFailureCause::repeated_cycle;
            }
        } else {
            run_length = 1;
        }
    }
    return ReflexionFailureCause::none;
}

ReflexionEvaluation evaluate_alfworld_trajectory(
    const ReflexionTrajectory& trajectory,
    bool environment_success,
    size_t repeated_cycle_limit,
    size_t action_limit) {
    if (environment_success) {
        return {1.0, true, "Environment reported task success."};
    }

    switch (detect_alfworld_failure(
        trajectory, repeated_cycle_limit, action_limit)) {
    case ReflexionFailureCause::repeated_cycle:
        return {0.0, false,
                "Repeated identical action and observation; planning is stuck."};
    case ReflexionFailureCause::action_limit:
        return {0.0, false, "Action limit exceeded; planning was inefficient."};
    case ReflexionFailureCause::none:
        return {0.0, false, "Environment has not reported task success."};
    }
    throw std::logic_error("Reflexion: unknown ALFWorld failure cause");
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
