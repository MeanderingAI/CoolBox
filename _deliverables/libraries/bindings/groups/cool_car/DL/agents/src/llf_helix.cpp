#include "llf_helix.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace agents {
namespace {

void validate(const LlfProblem& problem) {
    if (problem.num_actions == 0) {
        throw std::invalid_argument("LlfProblem: needs at least one action");
    }
    if (problem.hypotheses.empty()) {
        throw std::invalid_argument("LlfProblem: needs at least one hypothesis");
    }
    if (problem.truth >= problem.hypotheses.size()) {
        throw std::out_of_range("LlfProblem: truth index is out of range");
    }
    for (const LlfHypothesis& hypothesis : problem.hypotheses) {
        if (!hypothesis.reward || !hypothesis.feedback) {
            throw std::invalid_argument("LlfProblem: every hypothesis needs reward and feedback");
        }
    }
}

/// Best action under any of the given hypotheses, i.e. optimism.
size_t optimistic_action(const LlfProblem& problem, const std::vector<size_t>& candidates) {
    size_t best_action = 0;
    double best_value = -std::numeric_limits<double>::infinity();
    for (size_t action = 0; action < problem.num_actions; ++action) {
        double value = -std::numeric_limits<double>::infinity();
        for (size_t index : candidates) {
            value = std::max(value, problem.hypotheses[index].reward(action));
        }
        if (value > best_value) {
            best_value = value;
            best_action = action;
        }
    }
    return best_action;
}

} // namespace

double LlfProblem::true_reward(size_t action) const {
    return hypotheses[truth].reward(action);
}

std::string LlfProblem::true_feedback(size_t action) const {
    return hypotheses[truth].feedback(action);
}

double LlfProblem::optimal_reward() const {
    double best = -std::numeric_limits<double>::infinity();
    for (size_t action = 0; action < num_actions; ++action) {
        best = std::max(best, true_reward(action));
    }
    return best;
}

size_t transfer_eluder_dimension(const LlfProblem& problem, double epsilon, size_t max_actions) {
    validate(problem);
    const size_t n = problem.num_actions;
    if (n > max_actions) {
        throw std::invalid_argument("transfer_eluder_dimension: too many actions to enumerate");
    }
    const size_t hypothesis_count = problem.hypotheses.size();

    std::vector<std::vector<std::string>> feedback(hypothesis_count, std::vector<std::string>(n));
    std::vector<std::vector<double>> reward(hypothesis_count, std::vector<double>(n));
    for (size_t h = 0; h < hypothesis_count; ++h) {
        for (size_t a = 0; a < n; ++a) {
            feedback[h][a] = problem.hypotheses[h].feedback(a);
            reward[h][a] = problem.hypotheses[h].reward(a);
        }
    }

    // independent[mask][a]: after seeing the feedback from every action in
    // mask, can two surviving hypotheses still disagree about a's reward?
    const size_t states = static_cast<size_t>(1) << n;
    std::vector<std::vector<char>> independent(states, std::vector<char>(n, 0));
    for (size_t mask = 0; mask < states; ++mask) {
        for (size_t a = 0; a < n; ++a) {
            if (mask & (static_cast<size_t>(1) << a)) {
                continue;
            }
            bool found = false;
            for (size_t h = 0; h < hypothesis_count && !found; ++h) {
                for (size_t g = h + 1; g < hypothesis_count && !found; ++g) {
                    if (std::abs(reward[h][a] - reward[g][a]) <= epsilon) {
                        continue;
                    }
                    bool agree = true;
                    for (size_t seen = 0; seen < n && agree; ++seen) {
                        if ((mask & (static_cast<size_t>(1) << seen)) &&
                            feedback[h][seen] != feedback[g][seen]) {
                            agree = false;
                        }
                    }
                    if (agree) {
                        found = true;
                    }
                }
            }
            independent[mask][a] = found ? 1 : 0;
        }
    }

    // Longest chain of successively independent actions.
    std::vector<int> longest(states, -1);
    longest[0] = 0;
    size_t best = 0;
    for (size_t mask = 0; mask < states; ++mask) {
        if (longest[mask] < 0) {
            continue;
        }
        best = std::max(best, static_cast<size_t>(longest[mask]));
        for (size_t a = 0; a < n; ++a) {
            if (mask & (static_cast<size_t>(1) << a)) {
                continue;
            }
            if (!independent[mask][a]) {
                continue;
            }
            const size_t next = mask | (static_cast<size_t>(1) << a);
            longest[next] = std::max(longest[next], longest[mask] + 1);
        }
    }
    return best;
}

// ----------------------------------------------------------------------------
// HELiX
// ----------------------------------------------------------------------------

Helix::Helix(LlfProblem problem) : problem_(std::move(problem)) {
    validate(problem_);
    version_space_.resize(problem_.hypotheses.size());
    std::iota(version_space_.begin(), version_space_.end(), 0);
}

size_t Helix::select_action() const {
    if (version_space_.empty()) {
        throw std::runtime_error("Helix: the version space is empty, the class was misspecified");
    }
    return optimistic_action(problem_, version_space_);
}

void Helix::observe(size_t action, const std::string& feedback) {
    if (action >= problem_.num_actions) {
        throw std::out_of_range("Helix::observe: action is out of range");
    }
    std::vector<size_t> survivors;
    survivors.reserve(version_space_.size());
    for (size_t index : version_space_) {
        if (problem_.hypotheses[index].feedback(action) == feedback) {
            survivors.push_back(index);
        }
    }
    version_space_ = std::move(survivors);
}

size_t Helix::interact() {
    const size_t action = select_action();
    const double regret = problem_.optimal_reward() - problem_.true_reward(action);

    cumulative_regret_ += regret;
    if (regret > 0.0) {
        ++mistakes_;
    }
    ++rounds_;

    observe(action, problem_.true_feedback(action));
    return action;
}

// ----------------------------------------------------------------------------
// Memoryless baseline
// ----------------------------------------------------------------------------

MemorylessPrompting::MemorylessPrompting(LlfProblem problem) : problem_(std::move(problem)) {
    validate(problem_);
    candidates_.resize(problem_.hypotheses.size());
    std::iota(candidates_.begin(), candidates_.end(), 0);
}

size_t MemorylessPrompting::interact() {
    if (candidates_.empty()) {
        candidates_.resize(problem_.hypotheses.size());
        std::iota(candidates_.begin(), candidates_.end(), 0);
    }
    const size_t action = optimistic_action(problem_, candidates_);
    const double regret = problem_.optimal_reward() - problem_.true_reward(action);

    cumulative_regret_ += regret;
    if (regret > 0.0) {
        ++mistakes_;
    }
    ++rounds_;

    // Only the newest observation is retained.
    const std::string feedback = problem_.true_feedback(action);
    std::vector<size_t> survivors;
    for (size_t index = 0; index < problem_.hypotheses.size(); ++index) {
        if (problem_.hypotheses[index].feedback(action) == feedback) {
            survivors.push_back(index);
        }
    }
    candidates_ = std::move(survivors);
    return action;
}

// ----------------------------------------------------------------------------
// Instance builders
// ----------------------------------------------------------------------------

namespace {

/// Hypothesis i rewards action i and nothing else.
double preferred_action_reward(size_t preferred, size_t action) {
    return preferred == action ? 1.0 : 0.0;
}

} // namespace

LlfProblem make_reward_only_problem(size_t num_actions, size_t truth) {
    if (num_actions == 0) {
        throw std::invalid_argument("make_reward_only_problem: need at least one action");
    }
    LlfProblem problem;
    problem.num_actions = num_actions;
    problem.truth = truth;
    for (size_t preferred = 0; preferred < num_actions; ++preferred) {
        LlfHypothesis hypothesis;
        hypothesis.name = "prefers_" + std::to_string(preferred);
        hypothesis.reward = [preferred](size_t action) {
            return preferred_action_reward(preferred, action);
        };
        hypothesis.feedback = [preferred](size_t action) {
            return preferred_action_reward(preferred, action) > 0.0 ? std::string("reward:1")
                                                                    : std::string("reward:0");
        };
        problem.hypotheses.push_back(std::move(hypothesis));
    }
    validate(problem);
    return problem;
}

LlfProblem make_rich_feedback_problem(size_t num_actions, size_t truth) {
    if (num_actions == 0) {
        throw std::invalid_argument("make_rich_feedback_problem: need at least one action");
    }
    LlfProblem problem;
    problem.num_actions = num_actions;
    problem.truth = truth;
    for (size_t preferred = 0; preferred < num_actions; ++preferred) {
        LlfHypothesis hypothesis;
        hypothesis.name = "prefers_" + std::to_string(preferred);
        hypothesis.reward = [preferred](size_t action) {
            return preferred_action_reward(preferred, action);
        };
        hypothesis.feedback = [preferred](size_t action) {
            if (action == preferred) {
                return std::string("that is the best you can do");
            }
            return "try action " + std::to_string(preferred) + " instead";
        };
        problem.hypotheses.push_back(std::move(hypothesis));
    }
    validate(problem);
    return problem;
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
