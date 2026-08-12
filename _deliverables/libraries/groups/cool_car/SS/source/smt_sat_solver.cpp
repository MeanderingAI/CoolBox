#include "smt_sat_solver.h"

#include "matrix_dense.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cool_car::SS {

namespace {

constexpr double kTolerance = 1e-9;

} // namespace

std::size_t SmtSatSolver::add_variable() {
    return ++variable_count_;
}

void SmtSatSolver::add_clause(const std::vector<Literal>& clause) {
    for (Literal literal : clause) {
        if (literal == 0 || static_cast<std::size_t>(std::abs(literal)) > variable_count_) {
            throw std::invalid_argument("clause literal refers to an unknown variable");
        }
    }
    clauses_.push_back(clause);
}

void SmtSatSolver::add_linear_constraint(
    const std::vector<double>& coefficients,
    Relation relation,
    double bound) {
    if (coefficients.size() != variable_count_) {
        throw std::invalid_argument("linear constraint must contain one coefficient per variable");
    }
    linear_constraints_.push_back({coefficients, relation, bound});
}

SmtSatSolver::SolveResult SmtSatSolver::solve(const std::vector<Literal>& assumptions) const {
    SolveResult result;
    std::vector<int> assignment(variable_count_, -1);
    for (Literal literal : assumptions) {
        if (literal == 0 || static_cast<std::size_t>(std::abs(literal)) > variable_count_) {
            throw std::invalid_argument("assumption literal refers to an unknown variable");
        }

        const std::size_t variable = static_cast<std::size_t>(std::abs(literal) - 1);
        const int value = literal > 0 ? 1 : 0;
        if (assignment[variable] != -1 && assignment[variable] != value) {
            ++result.statistics.conflicts;
            return result;
        }
        assignment[variable] = value;
    }

    result.satisfiable = dpll(assignment, result.statistics);
    if (result.satisfiable) {
        result.assignment.reserve(variable_count_);
        for (int value : assignment) {
            result.assignment.push_back(value == 1);
        }
    }
    return result;
}

SmtSatSolver::SolveResult SmtSatSolver::maximize(
    const std::vector<double>& weights,
    opt::OptimizationAlgorithm* optimizer) const {
    if (!weights.empty() && weights.size() != variable_count_) {
        throw std::invalid_argument("objective must contain one weight per variable");
    }

    const std::vector<double> objective_weights = weights.empty()
        ? std::vector<double>(variable_count_, 0.0)
        : weights;

    if (optimizer != nullptr && variable_count_ > 0) {
        optimizer->optimize(
            [this, &objective_weights](const std::vector<double>& candidate) {
                if (candidate.size() != variable_count_) {
                    return std::numeric_limits<double>::max();
                }

                std::vector<bool> assignment(variable_count_);
                for (std::size_t index = 0; index < variable_count_; ++index) {
                    assignment[index] = candidate[index] >= 0.5;
                }
                if (!satisfies(assignment)) {
                    return std::numeric_limits<double>::max() / 2.0;
                }

                double score = 0.0;
                for (std::size_t index = 0; index < variable_count_; ++index) {
                    score -= objective_weights[index] * (assignment[index] ? 1.0 : 0.0);
                }
                return score;
            },
            std::vector<double>(variable_count_, 0.0));
    }

    SolveResult best;
    best.objective_value = -std::numeric_limits<double>::infinity();
    std::vector<bool> assignment(variable_count_, false);
    enumerate(0, assignment, objective_weights, best);
    return best;
}

bool SmtSatSolver::satisfies(const std::vector<bool>& assignment) const {
    return clauses_satisfied(assignment) && linear_constraints_satisfied(assignment);
}

bool SmtSatSolver::assumptions_satisfied(
    const std::vector<bool>& assignment,
    const std::vector<Literal>& assumptions) const {
    for (Literal literal : assumptions) {
        if (literal == 0 || static_cast<std::size_t>(std::abs(literal)) > variable_count_) {
            throw std::invalid_argument("assumption literal refers to an unknown variable");
        }
        const bool value = assignment[static_cast<std::size_t>(std::abs(literal) - 1)];
        if ((literal > 0 && !value) || (literal < 0 && value)) {
            return false;
        }
    }
    return true;
}

bool SmtSatSolver::clauses_satisfied(const std::vector<bool>& assignment) const {
    for (const auto& clause : clauses_) {
        bool clause_satisfied = false;
        for (Literal literal : clause) {
            const std::size_t variable = static_cast<std::size_t>(std::abs(literal) - 1);
            const bool value = assignment[variable];
            if ((literal > 0 && value) || (literal < 0 && !value)) {
                clause_satisfied = true;
                break;
            }
        }
        if (!clause_satisfied) {
            return false;
        }
    }
    return true;
}

bool SmtSatSolver::linear_constraints_satisfied(const std::vector<bool>& assignment) const {
    for (const auto& constraint : linear_constraints_) {
        mytrix::DenseMatrix coefficients(
            constraint.coefficients, static_cast<std::size_t>(1), variable_count_);
        mytrix::DenseMatrix values(assignment.size(), 1);
        for (std::size_t index = 0; index < assignment.size(); ++index) {
            values.at(index, 0) = assignment[index] ? 1.0 : 0.0;
        }

        const double value = (coefficients * values).at(0, 0);
        switch (constraint.relation) {
        case Relation::LessEqual:
            if (value > constraint.bound + kTolerance) return false;
            break;
        case Relation::GreaterEqual:
            if (value < constraint.bound - kTolerance) return false;
            break;
        case Relation::Equal:
            if (std::abs(value - constraint.bound) > kTolerance) return false;
            break;
        }
    }
    return true;
}

bool SmtSatSolver::dpll(std::vector<int>& assignment, SolveStatistics& statistics) const {
    if (!propagate(assignment, statistics)) {
        return false;
    }

    const auto unassigned = std::find(assignment.begin(), assignment.end(), -1);
    if (unassigned == assignment.end()) {
        std::vector<bool> model;
        model.reserve(variable_count_);
        for (int value : assignment) {
            model.push_back(value == 1);
        }
        if (!satisfies(model)) {
            ++statistics.conflicts;
            return false;
        }
        return true;
    }

    const std::size_t variable = static_cast<std::size_t>(unassigned - assignment.begin());
    ++statistics.decisions;
    for (int value : {0, 1}) {
        std::vector<int> branch = assignment;
        branch[variable] = value;
        if (dpll(branch, statistics)) {
            assignment = std::move(branch);
            return true;
        }
    }
    return false;
}

bool SmtSatSolver::propagate(std::vector<int>& assignment, SolveStatistics& statistics) const {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& clause : clauses_) {
            bool clause_satisfied = false;
            Literal unit_literal = 0;
            std::size_t unassigned_count = 0;
            for (Literal literal : clause) {
                const std::size_t variable = static_cast<std::size_t>(std::abs(literal) - 1);
                const int value = assignment[variable];
                if (value == -1) {
                    unit_literal = literal;
                    ++unassigned_count;
                } else if ((literal > 0 && value == 1) || (literal < 0 && value == 0)) {
                    clause_satisfied = true;
                    break;
                }
            }

            if (clause_satisfied) {
                continue;
            }
            if (unassigned_count == 0) {
                ++statistics.conflicts;
                return false;
            }
            if (unassigned_count == 1) {
                const std::size_t variable = static_cast<std::size_t>(std::abs(unit_literal) - 1);
                const int value = unit_literal > 0 ? 1 : 0;
                if (assignment[variable] == -1) {
                    assignment[variable] = value;
                    ++statistics.propagations;
                    changed = true;
                }
            }
        }

        for (std::size_t variable = 0; variable < variable_count_; ++variable) {
            if (assignment[variable] != -1) {
                continue;
            }

            bool appears_positive = false;
            bool appears_negative = false;
            for (const auto& clause : clauses_) {
                bool clause_satisfied = false;
                for (Literal literal : clause) {
                    const std::size_t literal_variable = static_cast<std::size_t>(std::abs(literal) - 1);
                    const int value = assignment[literal_variable];
                    if ((literal > 0 && value == 1) || (literal < 0 && value == 0)) {
                        clause_satisfied = true;
                        break;
                    }
                }
                if (clause_satisfied) {
                    continue;
                }

                for (Literal literal : clause) {
                    if (static_cast<std::size_t>(std::abs(literal) - 1) == variable) {
                        appears_positive = appears_positive || literal > 0;
                        appears_negative = appears_negative || literal < 0;
                    }
                }
            }

            if (appears_positive != appears_negative) {
                assignment[variable] = appears_positive ? 1 : 0;
                ++statistics.propagations;
                changed = true;
            }
        }
    }
    return true;
}

void SmtSatSolver::enumerate(
    std::size_t variable,
    std::vector<bool>& assignment,
    const std::vector<double>& weights,
    SolveResult& best) const {
    if (variable == variable_count_) {
        if (!satisfies(assignment)) {
            return;
        }

        double score = 0.0;
        for (std::size_t index = 0; index < variable_count_; ++index) {
            score += weights[index] * (assignment[index] ? 1.0 : 0.0);
        }
        if (!best.satisfiable || score > best.objective_value) {
            best.satisfiable = true;
            best.assignment = assignment;
            best.objective_value = score;
        }
        return;
    }

    assignment[variable] = false;
    enumerate(variable + 1, assignment, weights, best);
    assignment[variable] = true;
    enumerate(variable + 1, assignment, weights, best);
}

std::vector<std::vector<bool>> SmtSatSolver::enumerate_models(
    std::size_t limit,
    const std::vector<Literal>& assumptions) const {
    std::vector<std::vector<bool>> models;
    if (limit == 0) {
        return models;
    }

    std::vector<bool> assignment(variable_count_, false);
    enumerate_models(0, assignment, limit, assumptions, models);
    return models;
}

SmtSatSolver::ModelCountResult SmtSatSolver::count_models(
    std::size_t limit,
    const std::vector<Literal>& assumptions) const {
    if (limit == std::numeric_limits<std::size_t>::max()) {
        const auto models = enumerate_models(limit, assumptions);
        return {models.size(), true};
    }

    const auto models = enumerate_models(limit + 1, assumptions);
    return {std::min(models.size(), limit), models.size() <= limit};
}

SmtSatSolver::ProbabilityResult SmtSatSolver::probability(
    const std::vector<double>& true_probabilities,
    const std::vector<Literal>& assumptions) const {
    if (true_probabilities.size() != variable_count_) {
        throw std::invalid_argument("probabilities must contain one value per variable");
    }
    for (double probability : true_probabilities) {
        if (probability < 0.0 || probability > 1.0) {
            throw std::invalid_argument("variable probabilities must be in [0, 1]");
        }
    }

    const auto models = enumerate_models(static_cast<std::size_t>(-1), assumptions);
    ProbabilityResult result;
    result.models = {models.size(), true};
    for (const auto& model : models) {
        double model_probability = 1.0;
        for (std::size_t index = 0; index < variable_count_; ++index) {
            model_probability *= model[index]
                ? true_probabilities[index]
                : 1.0 - true_probabilities[index];
        }
        result.probability += model_probability;
    }
    return result;
}

void SmtSatSolver::enumerate_models(
    std::size_t variable,
    std::vector<bool>& assignment,
    std::size_t limit,
    const std::vector<Literal>& assumptions,
    std::vector<std::vector<bool>>& models) const {
    if (models.size() == limit) {
        return;
    }
    if (variable == variable_count_) {
        if (satisfies(assignment) && assumptions_satisfied(assignment, assumptions)) {
            models.push_back(assignment);
        }
        return;
    }

    assignment[variable] = false;
    enumerate_models(variable + 1, assignment, limit, assumptions, models);
    assignment[variable] = true;
    enumerate_models(variable + 1, assignment, limit, assumptions, models);
}

} // namespace cool_car::SS