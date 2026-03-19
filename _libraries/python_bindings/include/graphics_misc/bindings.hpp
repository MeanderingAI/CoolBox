#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_graphics(py::module_& parent_module);
void bind_misc(py::module_& parent_module);