#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include "../../../groups/cool_car/MATH/pde_solver.hpp"
#include "../../../groups/cool_car/MATH/spde.hpp"
#include "../../../groups/cool_car/MATRIX/headers/matrix_dense.h"

namespace py = pybind11;

void bind_pde_spde(py::module_ &m);
