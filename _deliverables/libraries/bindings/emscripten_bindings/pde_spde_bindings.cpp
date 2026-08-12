#include <emscripten/bind.h>
#include "_deliverables/libraries/groups/cool_car/MATH/pde_solver.hpp"
#include "_deliverables/libraries/groups/cool_car/MATH/spde.hpp"
#include "_deliverables/libraries/groups/cool_car/MATRIX/headers/matrix_dense.h"
#include <random>
#include <string>
#include <vector>

using namespace emscripten;
using namespace cool_car::MATH;

static mytrix::DenseMatrix to_dense_matrix_1d(const std::vector<double>& values) {
    mytrix::DenseMatrix matrix(static_cast<int>(values.size()), 1);
    for (size_t index = 0; index < values.size(); ++index) {
        matrix(static_cast<int>(index), 0) = values[index];
    }
    return matrix;
}

static std::vector<double> from_dense_matrix_1d(const mytrix::DenseMatrix& matrix) {
    std::vector<double> values(static_cast<size_t>(matrix.rows()));
    for (int row = 0; row < matrix.rows(); ++row) {
        values[static_cast<size_t>(row)] = matrix(row, 0);
    }
    return values;
}

static mytrix::DenseMatrix to_dense_matrix_2d(const std::vector<std::vector<double>>& values) {
    const int rows = static_cast<int>(values.size());
    const int cols = rows > 0 ? static_cast<int>(values[0].size()) : 0;
    mytrix::DenseMatrix matrix(rows, cols);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            matrix(row, col) = values[static_cast<size_t>(row)][static_cast<size_t>(col)];
        }
    }
    return matrix;
}

static std::vector<std::vector<double>> from_dense_matrix_2d(const mytrix::DenseMatrix& matrix) {
    std::vector<std::vector<double>> values(
        static_cast<size_t>(matrix.rows()),
        std::vector<double>(static_cast<size_t>(matrix.cols()))
    );

    for (int row = 0; row < matrix.rows(); ++row) {
        for (int col = 0; col < matrix.cols(); ++col) {
            values[static_cast<size_t>(row)][static_cast<size_t>(col)] = matrix(row, col);
        }
    }

    return values;
}

std::vector<double> solve_pde_1d(const std::vector<double>& u0, double alpha, double dx, double dt, int steps) {
    const auto result = PDESolver::solve_heat_eq(to_dense_matrix_1d(u0), alpha, dx, dt, steps);
    return from_dense_matrix_1d(result);
}

std::vector<std::vector<double>> solve_pde_2d(const std::vector<std::vector<double>>& u0, double alpha, double dx, double dy, double dt, int steps) {
    const auto result = PDESolver::solve_heat_eq_2d(to_dense_matrix_2d(u0), alpha, dx, dy, dt, steps);
    return from_dense_matrix_2d(result);
}

std::vector<double> solve_spde_1d(const std::vector<double>& u0, double alpha, double sigma, double dx, double dt, int steps, int seed) {
    std::mt19937 rng(seed);
    const auto result = SPDESolver::solve_stochastic_heat_eq(to_dense_matrix_1d(u0), alpha, sigma, dx, dt, steps, rng);
    return from_dense_matrix_1d(result);
}

std::vector<std::vector<double>> solve_spde_2d(const std::vector<std::vector<double>>& u0, double alpha, double sigma, double dx, double dy, double dt, int steps, int seed) {
    std::mt19937 rng(seed);
    const auto result = SPDESolver::solve_stochastic_heat_eq_2d(to_dense_matrix_2d(u0), alpha, sigma, dx, dy, dt, steps, rng);
    return from_dense_matrix_2d(result);
}

EMSCRIPTEN_BINDINGS(pde_spde_module) {
    function("solve_pde_1d", &solve_pde_1d);
    function("solve_pde_2d", &solve_pde_2d);
    function("solve_spde_1d", &solve_spde_1d);
    function("solve_spde_2d", &solve_spde_2d);
}
