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

    struct SolveResult {
        bool satisfiable = false;
        std::vector<bool> assignment;
        double objective_value = 0.0;
    };

    std::size_t add_variable();
    void add_clause(const std::vector<Literal>& clause);
    void add_linear_constraint(
        const std::vector<double>& coefficients,
        Relation relation,
        double bound);

    SolveResult solve() const;
    SolveResult maximize(
        const std::vector<double>& weights,
        opt::OptimizationAlgorithm* optimizer = nullptr) const;

private:
    struct LinearConstraint {
        std::vector<double> coefficients;
        Relation relation;
        double bound;
    };

    bool satisfies(const std::vector<bool>& assignment) const;
    bool clauses_satisfied(const std::vector<bool>& assignment) const;
    bool linear_constraints_satisfied(const std::vector<bool>& assignment) const;
    void enumerate(
        std::size_t variable,
        std::vector<bool>& assignment,
        const std::vector<double>& weights,
        SolveResult& best) const;

    std::size_t variable_count_ = 0;
    std::vector<std::vector<Literal>> clauses_;
    std::vector<LinearConstraint> linear_constraints_;
};

} // namespace cool_car::SS