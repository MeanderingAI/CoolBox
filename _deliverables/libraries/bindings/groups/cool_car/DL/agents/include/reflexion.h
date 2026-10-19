#ifndef ML_DEEP_LEARNING_AGENTS_REFLEXION_H
#define ML_DEEP_LEARNING_AGENTS_REFLEXION_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// @brief Reflexion: reinforcement through verbal reflection.
///
/// Implements Shinn et al., "Reflexion: Language Agents with Verbal
/// Reinforcement Learning" (arXiv:2303.11366), citation key
/// @c shinn_reflexion_2023.
///
/// Reflexion does not update model weights. An Actor creates a trajectory, an
/// Evaluator assigns task feedback, and a Self-Reflection model turns a failed
/// trajectory into verbal advice. The advice is retained in bounded episodic
/// memory and conditions the Actor's next trial. The model calls are callbacks
/// so callers can connect an LLM, a local model, or deterministic test doubles
/// without adding a model-runtime dependency to CoolBox.

struct ReflexionStep {
    std::string action;
    std::string observation;
};

/// Short-term memory for one trial.
struct ReflexionTrajectory {
    std::vector<ReflexionStep> steps;
    /// Final generation for single-step reasoning/programming tasks.
    std::string output;
};

/// Scalar/environment feedback produced by M_e in the paper.
struct ReflexionEvaluation {
    double reward = 0.0;
    bool passed = false;
    std::string feedback;
};

using ReflexionMemory = std::vector<std::string>;
using ReflexionActor =
    std::function<ReflexionTrajectory(const std::string&, const ReflexionMemory&)>;
using ReflexionEvaluator =
    std::function<ReflexionEvaluation(const std::string&, const ReflexionTrajectory&)>;
using ReflexionSelfReflection = std::function<std::string(
    const std::string&,
    const ReflexionTrajectory&,
    const ReflexionEvaluation&,
    const ReflexionMemory&)>;

struct ReflexionTrial {
    ReflexionTrajectory trajectory;
    ReflexionEvaluation evaluation;
    /// Empty on a passing trial because no subsequent policy needs a hint.
    std::string reflection;
};

struct ReflexionResult {
    bool passed = false;
    std::vector<ReflexionTrial> trials;
    /// Snapshot of long-term memory after the run, oldest entry first.
    ReflexionMemory memory;
};

class ReflexionAgent {
public:
    struct Config {
        size_t max_trials = 12;
        /// Paper notation Omega; experiments usually use one to three.
        size_t memory_capacity = 3;
    };

    ReflexionAgent(ReflexionActor actor,
                   ReflexionEvaluator evaluator,
                   ReflexionSelfReflection self_reflection);
    ReflexionAgent(ReflexionActor actor,
                   ReflexionEvaluator evaluator,
                   ReflexionSelfReflection self_reflection,
                   Config config);

    /// Executes Actor -> Evaluator -> (on failure) Reflection until pass or the
    /// trial budget is exhausted. Existing episodic memory is preserved.
    ReflexionResult run(const std::string& task);

    const ReflexionMemory& memory() const { return memory_; }
    void clear_memory() { memory_.clear(); }

private:
    void remember(std::string reflection);

    ReflexionActor actor_;
    ReflexionEvaluator evaluator_;
    ReflexionSelfReflection self_reflection_;
    Config config_;
    ReflexionMemory memory_;
};

/// Failure modes used by the paper's deterministic ALFWorld evaluator:
/// repeating an identical action/observation more than the cycle limit, or
/// exceeding the action budget.
enum class ReflexionFailureCause {
    none,
    repeated_cycle,
    action_limit
};

ReflexionFailureCause detect_alfworld_failure(
    const ReflexionTrajectory& trajectory,
    size_t repeated_cycle_limit = 3,
    size_t action_limit = 30);

/// Builds the sparse binary evaluation used in the ALFWorld experiments.
/// Environment success takes precedence over heuristic failure detection.
ReflexionEvaluation evaluate_alfworld_trajectory(
    const ReflexionTrajectory& trajectory,
    bool environment_success,
    size_t repeated_cycle_limit = 3,
    size_t action_limit = 30);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_REFLEXION_H
