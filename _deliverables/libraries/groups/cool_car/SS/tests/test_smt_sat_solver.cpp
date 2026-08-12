#include "tyst_framework.hpp"

#include "hill_climbing.h"
#include "smt_sat_solver.h"

using cool_car::SS::SmtSatSolver;

TEST(SmtSatSolverTest, SolvesCnfWithLinearIntegerTheoryConstraints) {
    SmtSatSolver solver;
    const auto alarm = solver.add_variable();
    const auto door_open = solver.add_variable();
    const auto window_open = solver.add_variable();

    solver.add_clause({static_cast<int>(alarm)});
    solver.add_clause({-static_cast<int>(door_open), static_cast<int>(window_open)});
    solver.add_linear_constraint({1.0, 1.0, 1.0}, SmtSatSolver::Relation::LessEqual, 2.0);

    const auto result = solver.solve();

    ASSERT_TRUE(result.satisfiable);
    ASSERT_EQ(result.assignment.size(), static_cast<std::size_t>(3));
    EXPECT_TRUE(result.assignment[0]);
}

TEST(SmtSatSolverTest, DetectsUnsatisfiableFormula) {
    SmtSatSolver solver;
    const auto enabled = solver.add_variable();
    solver.add_clause({static_cast<int>(enabled)});
    solver.add_clause({-static_cast<int>(enabled)});

    const auto result = solver.solve();

    EXPECT_FALSE(result.satisfiable);
}

TEST(SmtSatSolverTest, MaximizesWeightedModelAndAcceptsOptWarmStart) {
    SmtSatSolver solver;
    const auto first = solver.add_variable();
    const auto second = solver.add_variable();
    solver.add_clause({static_cast<int>(first), static_cast<int>(second)});
    solver.add_linear_constraint({1.0, 1.0}, SmtSatSolver::Relation::LessEqual, 1.0);

    opt::HillClimbing::Config config;
    config.dimensions = 2;
    config.max_iterations = 20;
    config.max_restarts = 1;
    config.seed = 7;
    opt::HillClimbing optimizer(config);

    const auto result = solver.maximize({2.0, 5.0}, &optimizer);

    ASSERT_TRUE(result.satisfiable);
    EXPECT_FALSE(result.assignment[0]);
    EXPECT_TRUE(result.assignment[1]);
    EXPECT_EQ(result.objective_value, 5.0);
}