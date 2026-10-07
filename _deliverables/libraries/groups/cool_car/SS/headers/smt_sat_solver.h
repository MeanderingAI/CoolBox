#pragma once

#include "optimization_algorithm.h"

#include <cstddef>
#include <vector>

namespace cool_car::SS {

class SmtSatSolver {
public:
    using Literal = int;

    enum class Relation {
        LessEqual,
        GreaterEqual,
        Equal
    };

    struct SolveStatistics {
        std::size_t decisions = 0;
        std::size_t propagations = 0;
        std::size_t conflicts = 0;
    };

    struct SolveResult {
        bool satisfiable = false;
        std::vector<bool> assignment;
        double objective_value = 0.0;
        SolveStatistics statistics;
    };

    struct ModelCountResult {
        std::size_t count = 0;
        bool complete = true;
    };

    struct ProbabilityResult {
        ModelCountResult models;
        double probability = 0.0;
    };

    std::size_t add_variable();
    void add_clause(const std::vector<Literal>& clause);
    void add_linear_constraint(
        const std::vector<double>& coefficients,
        Relation relation,
        double bound);

    SolveResult solve(const std::vector<Literal>& assumptions = {}) const;
    SolveResult maximize(
        const std::vector<double>& weights,
        opt::OptimizationAlgorithm* optimizer = nullptr) const;
    std::vector<std::vector<bool>> enumerate_models(
        std::size_t limit,
        const std::vector<Literal>& assumptions = {}) const;
    ModelCountResult count_models(
        std::size_t limit = static_cast<std::size_t>(-1),
        const std::vector<Literal>& assumptions = {}) const;
    ProbabilityResult probability(
        const std::vector<double>& true_probabilities,
        const std::vector<Literal>& assumptions = {}) const;

private:
    struct LinearConstraint {
        std::vector<double> coefficients;
        Relation relation;
        double bound;
    };

    bool satisfies(const std::vector<bool>& assignment) const;
    bool assumptions_satisfied(
        const std::vector<bool>& assignment,
        const std::vector<Literal>& assumptions) const;
    bool clauses_satisfied(const std::vector<bool>& assignment) const;
    bool linear_constraints_satisfied(const std::vector<bool>& assignment) const;
    bool dpll(std::vector<int>& assignment, SolveStatistics& statistics) const;
    bool propagate(std::vector<int>& assignment, SolveStatistics& statistics) const;
    void enumerate(
        std::size_t variable,
        std::vector<bool>& assignment,
        const std::vector<double>& weights,
        SolveResult& best) const;
    void enumerate_models(
        std::size_t variable,
        std::vector<bool>& assignment,
        std::size_t limit,
        const std::vector<Literal>& assumptions,
        std::vector<std::vector<bool>>& models) const;

    std::size_t variable_count_ = 0;
    std::vector<std::vector<Literal>> clauses_;
    std::vector<LinearConstraint> linear_constraints_;
};

} // namespace cool_car::SS