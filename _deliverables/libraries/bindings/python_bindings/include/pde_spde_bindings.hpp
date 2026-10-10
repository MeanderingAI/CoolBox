#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include "MATH/pde_solver.hpp"
#include "MATH/spde.hpp"
#include "MATRIX/headers/matrix_dense.h"

namespace py = pybind11;

void bind_pde_spde(py::module_ &m);
