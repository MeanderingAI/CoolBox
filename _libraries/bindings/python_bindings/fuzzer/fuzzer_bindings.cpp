#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "security/fuzzer/fuzzer.h"

namespace py = pybind11;

PYBIND11_MODULE(fuzzer_bindings, m) {
    m.doc() = "Python bindings for Fuzzer library";
    // Example: expose a function or class from fuzzer.h
    // m.def("function_name", &security::fuzzer::function_name);
}
