#include <emscripten/bind.h>
#include "_deliverables/libraries/groups/cool_car/MATH/pde_solver.hpp"
#include "_deliverables/libraries/groups/cool_car/MATH/spde.hpp"
#include <string>
#include <vector>

using namespace emscripten;
using namespace cool_car;

// Simple wrapper for PDE solver (mytrix backend only for now)
std::vector<double> solve_pde_1d(const std::vector<double>& u0, double dx, double dt, int steps) {
    return PDE::solve_1d(u0, dx, dt, steps);
}

std::vector<std::vector<double>> solve_pde_2d(const std::vector<std::vector<double>>& u0, double dx, double dy, double dt, int steps) {
    return PDE::solve_2d(u0, dx, dy, dt, steps);
}

std::vector<double> solve_spde_1d(const std::vector<double>& u0, double dx, double dt, int steps, double noise) {
    return SPDE::solve_1d(u0, dx, dt, steps, noise);
}

std::vector<std::vector<double>> solve_spde_2d(const std::vector<std::vector<double>>& u0, double dx, double dy, double dt, int steps, double noise) {
    return SPDE::solve_2d(u0, dx, dy, dt, steps, noise);
}

EMSCRIPTEN_BINDINGS(pde_spde_module) {
    function("solve_pde_1d", &solve_pde_1d);
    function("solve_pde_2d", &solve_pde_2d);
    function("solve_spde_1d", &solve_spde_1d);
    function("solve_spde_2d", &solve_spde_2d);
}
