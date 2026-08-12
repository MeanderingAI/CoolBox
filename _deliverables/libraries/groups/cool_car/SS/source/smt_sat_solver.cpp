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

SmtSatSolver::SolveResult SmtSatSolver::solve() const {
    return maximize({});
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

} // namespace cool_car::SS