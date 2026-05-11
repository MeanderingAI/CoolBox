#ifndef COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_MISC_BINDINGS_HPP
#define COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_MISC_BINDINGS_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_graphics(py::module_& parent_module);
void bind_misc(py::module_& parent_module);
#endif  // COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_MISC_BINDINGS_HPP