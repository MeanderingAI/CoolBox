#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_synthetic_data(py::module_& parent_module);
