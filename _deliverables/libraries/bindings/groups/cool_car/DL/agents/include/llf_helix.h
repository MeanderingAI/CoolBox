#ifndef ML_DEEP_LEARNING_AGENTS_LLF_HELIX_H
#define ML_DEEP_LEARNING_AGENTS_LLF_HELIX_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// @brief Learning from Language Feedback (LLF) and the HELiX algorithm.
///
/// Implements Xu et al., "Formalizing Learning from Language Feedback with
/// Provable Guarantees", ICML 2026 (arXiv:2506.10341), citation key
/// @c xu_llf_helix_2025.
///
/// The learner never observes a reward. It picks an action, receives language
/// feedback, and must still converge on the action a latent reward function
/// prefers. The @em transfer @em eluder @em dimension measures how often the
/// learner can still be surprised after everything it has seen, and HELiX is a
/// no-regret algorithm whose performance scales with that dimension.

/// One candidate explanation of the environment: what it rewards and what it
/// says. Both are deterministic functions of the action.
struct LlfHypothesis {
    std::string name;
    std::function<double(size_t)> reward;
    std::function<std::string(size_t)> feedback;
};

/// A finite LLF instance.
struct LlfProblem {
    size_t num_actions = 0;
    std::vector<LlfHypothesis> hypotheses;
    /// Index of the hypothesis that actually governs the environment.
    size_t truth = 0;

    double true_reward(size_t action) const;
    std::string true_feedback(size_t action) const;
    /// Reward of the best action under the truth.
    double optimal_reward() const;
};

/// Longest sequence of actions in which every action is still able to surprise
/// a learner that has already seen the feedback from all its predecessors.
///
/// An action is independent of a set when two hypotheses agree on the feedback
/// everywhere in that set yet disagree, by more than @p epsilon, on the reward
/// of the action. Because independence depends only on the set of predecessors,
/// the longest sequence is computed exactly by dynamic programming over
/// subsets; @p max_actions guards the 2^n state space.
size_t transfer_eluder_dimension(const LlfProblem& problem,
                                 double epsilon = 1e-9,
                                 size_t max_actions = 18);

/// HELiX: keep every hypothesis still consistent with the observed feedback and
/// act optimistically within that version space.
class Helix {
public:
    explicit Helix(LlfProblem problem);

    /// Optimistic choice: the action some surviving hypothesis rates highest.
    size_t select_action() const;

    /// Eliminates every hypothesis that would not have produced @p feedback.
    void observe(size_t action, const std::string& feedback);

    /// One interaction against the true environment. Returns the action taken.
    size_t interact();

    const std::vector<size_t>& version_space() const { return version_space_; }
    size_t rounds() const { return rounds_; }
    double cumulative_regret() const { return cumulative_regret_; }
    /// Rounds in which a strictly suboptimal action was chosen.
    size_t mistakes() const { return mistakes_; }
    bool identified() const { return version_space_.size() <= 1; }

private:
    LlfProblem problem_;
    std::vector<size_t> version_space_;
    size_t rounds_ = 0;
    size_t mistakes_ = 0;
    double cumulative_regret_ = 0.0;
};

/// Baseline standing in for repeatedly prompting an LLM: it reasons only from
/// the most recent feedback and forgets everything before it. On instances
/// where feedback is not self-revealing this oscillates forever.
class MemorylessPrompting {
public:
    explicit MemorylessPrompting(LlfProblem problem);

    size_t interact();

    size_t rounds() const { return rounds_; }
    double cumulative_regret() const { return cumulative_regret_; }
    size_t mistakes() const { return mistakes_; }

private:
    LlfProblem problem_;
    std::vector<size_t> candidates_;
    size_t rounds_ = 0;
    size_t mistakes_ = 0;
    double cumulative_regret_ = 0.0;
};

// ----------------------------------------------------------------------------
// Instance builders
// ----------------------------------------------------------------------------

/// Each hypothesis prefers a different action and the feedback only reports the
/// reward that was collected. This is effectively bandit feedback, so the
/// learner has to eliminate the actions one at a time.
LlfProblem make_reward_only_problem(size_t num_actions, size_t truth);

/// Same latent rewards, but the feedback is a sentence that names a better
/// action when the chosen one is wrong. A single interaction is now enough,
/// which is the exponential speed-up rich language feedback can buy.
LlfProblem make_rich_feedback_problem(size_t num_actions, size_t truth);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_LLF_HELIX_H
