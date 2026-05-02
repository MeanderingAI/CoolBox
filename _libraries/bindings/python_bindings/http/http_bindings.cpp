#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "dataformats/http/http.h"

namespace py = pybind11;

PYBIND11_MODULE(http_bindings, m) {
    m.doc() = "Python bindings for HTTP library";
    // Example: expose a function or class from http.h
    // m.def("function_name", &dataformats::http::function_name);
}
