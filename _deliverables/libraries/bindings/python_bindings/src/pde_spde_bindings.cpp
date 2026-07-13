#include "../include/pde_spde_bindings.hpp"

#if __has_include(<pde_solver.hpp>) && __has_include(<spde.hpp>)
#include <pde_solver.hpp>
#include <spde.hpp>
#define COOLBOX_HAS_PDE_SPDE_HEADERS 1
#else
#define COOLBOX_HAS_PDE_SPDE_HEADERS 0
#endif

namespace py = pybind11;

#if COOLBOX_HAS_PDE_SPDE_HEADERS
using namespace cool_car::MATH;
#endif

void bind_pde_spde(py::module_ &m) {
#if COOLBOX_HAS_PDE_SPDE_HEADERS
    py::module_ pde_mod = m.def_submodule("pde", "PDE solvers");
    py::module_ spde_mod = m.def_submodule("spde", "SPDE solvers");

    // mytrix::DenseMatrix binding (opaque, not as numpy)
    py::class_<mytrix::DenseMatrix>(pde_mod, "DenseMatrix")
        .def(py::init<int, int>())
        .def("rows", &mytrix::DenseMatrix::rows)
        .def("cols", &mytrix::DenseMatrix::cols)
        .def("__getitem__", [](const mytrix::DenseMatrix &m, std::pair<int, int> idx) { return m(idx.first, idx.second); })
        .def("__setitem__", [](mytrix::DenseMatrix &m, std::pair<int, int> idx, double v) { m(idx.first, idx.second) = v; });

    // PDE solvers
    pde_mod.def("solve_heat_eq", &PDESolver::solve_heat_eq,
        py::arg("u0"), py::arg("alpha"), py::arg("dx"), py::arg("dt"), py::arg("steps"),
        "Solve 1D heat equation (mytrix backend)");
    pde_mod.def("solve_heat_eq_2d", &PDESolver::solve_heat_eq_2d,
        py::arg("u0"), py::arg("alpha"), py::arg("dx"), py::arg("dy"), py::arg("dt"), py::arg("steps"),
        "Solve 2D heat equation (mytrix backend)");
    // TODO: Add xarray backend and ND support

    // SPDE solvers
    spde_mod.def("solve_stochastic_heat_eq", [](const mytrix::DenseMatrix& u0, double alpha, double sigma, double dx, double dt, int steps, int seed) {
        std::mt19937 rng(seed);
        return SPDESolver::solve_stochastic_heat_eq(u0, alpha, sigma, dx, dt, steps, rng);
    }, py::arg("u0"), py::arg("alpha"), py::arg("sigma"), py::arg("dx"), py::arg("dt"), py::arg("steps"), py::arg("seed") = 42,
    "Solve 1D stochastic heat equation (mytrix backend)");
    spde_mod.def("solve_stochastic_heat_eq_2d", [](const mytrix::DenseMatrix& u0, double alpha, double sigma, double dx, double dy, double dt, int steps, int seed) {
        std::mt19937 rng(seed);
        return SPDESolver::solve_stochastic_heat_eq_2d(u0, alpha, sigma, dx, dy, dt, steps, rng);
    }, py::arg("u0"), py::arg("alpha"), py::arg("sigma"), py::arg("dx"), py::arg("dy"), py::arg("dt"), py::arg("steps"), py::arg("seed") = 42,
    "Solve 2D stochastic heat equation (mytrix backend)");
    // TODO: Add xarray backend and ND support
#else
    py::module_ pde_mod = m.def_submodule("pde", "PDE solvers");
    py::module_ spde_mod = m.def_submodule("spde", "SPDE solvers");
    pde_mod.doc() = "PDE bindings unavailable in this build artifact.";
    spde_mod.doc() = "SPDE bindings unavailable in this build artifact.";
#endif
}
