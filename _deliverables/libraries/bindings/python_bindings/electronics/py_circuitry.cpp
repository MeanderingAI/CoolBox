#include "circuitry.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

namespace py = pybind11;
using namespace circuitry;

CircuitSolution solve_circuit_json(const std::string& json_str) {
    Circuit circuit = Circuit::from_json(json_str);
    CircuitSolver solver(circuit);
    return solver.solve();
}

PYBIND11_MODULE(py_circuitry, m) {
    py::class_<ComponentResult>(m, "ComponentResult")
        .def_readonly("label", &ComponentResult::label)
        .def_readonly("type_name", &ComponentResult::type_name)
        .def_readonly("resistance", &ComponentResult::resistance)
        .def_readonly("voltage_drop", &ComponentResult::voltage_drop)
        .def_readonly("current", &ComponentResult::current)
        .def_readonly("power", &ComponentResult::power)
        .def_readonly("emf", &ComponentResult::emf)
        .def_readonly("internal_resistance", &ComponentResult::internal_resistance)
        .def_readonly("terminal_voltage", &ComponentResult::terminal_voltage);

    py::class_<CircuitSolution>(m, "CircuitSolution")
        .def_readonly("node_voltages", &CircuitSolution::node_voltages)
        .def_readonly("component_results", &CircuitSolution::component_results);

    m.def("solve_circuit_json", &solve_circuit_json, "Solve a circuit from a JSON string");
}

