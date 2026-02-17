#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "curcuitry.h"

namespace py = pybind11;
using namespace curcuitry;

PYBIND11_MODULE(curcuitry, m) {
    m.doc() = "Circuit Maker & Solver – Python Bindings";

    // ── Point ───────────────────────────────────────────────────────
    py::class_<Point>(m, "Point")
        .def(py::init<double, double>(), py::arg("x") = 0, py::arg("y") = 0)
        .def_readwrite("x", &Point::x)
        .def_readwrite("y", &Point::y)
        .def("__eq__", &Point::operator==)
        .def("__repr__", [](const Point& p) {
            return "Point(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
        });

    // ── ComponentType enum ──────────────────────────────────────────
    py::enum_<ComponentType>(m, "ComponentType")
        .value("BATTERY",  ComponentType::BATTERY)
        .value("RESISTOR", ComponentType::RESISTOR)
        .value("WIRE",     ComponentType::WIRE);

    // ── Component (base) ────────────────────────────────────────────
    py::class_<Component, std::shared_ptr<Component>>(m, "Component")
        .def("type",      &Component::type)
        .def("type_name", &Component::type_name)
        .def("node1",     &Component::node1)
        .def("node2",     &Component::node2)
        .def("label",     &Component::label)
        .def("value",     &Component::value)
        .def("to_string", &Component::to_string)
        .def_static("parse_numeric", &Component::parse_numeric)
        .def("__repr__",  &Component::to_string);

    // ── Battery ─────────────────────────────────────────────────────
    py::class_<Battery, Component, std::shared_ptr<Battery>>(m, "Battery")
        .def(py::init<double, double, double, double,
                       const std::string&, const std::string&,
                       double, double>(),
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label"), py::arg("value"),
             py::arg("voltage"), py::arg("internal_resistance") = 0.0)
        .def_static("from_fields", &Battery::from_fields,
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label"), py::arg("value_str"),
             py::arg("resistance_str") = "")
        .def("voltage",             &Battery::voltage)
        .def("internal_resistance", &Battery::internal_resistance)
        .def("__repr__",            &Battery::to_string);

    // ── Resistor ────────────────────────────────────────────────────
    py::class_<Resistor, Component, std::shared_ptr<Resistor>>(m, "Resistor")
        .def(py::init<double, double, double, double,
                       const std::string&, const std::string&,
                       double>(),
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label"), py::arg("value"),
             py::arg("resistance"))
        .def_static("from_fields", &Resistor::from_fields,
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label"), py::arg("value_str"))
        .def("resistance", &Resistor::resistance)
        .def("__repr__",   &Resistor::to_string);

    // ── Wire ────────────────────────────────────────────────────────
    py::class_<Wire, Component, std::shared_ptr<Wire>>(m, "Wire")
        .def(py::init<double, double, double, double,
                       const std::string&, const std::string&>(),
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label") = "", py::arg("value") = "")
        .def_static("from_fields", &Wire::from_fields,
             py::arg("x1"), py::arg("y1"),
             py::arg("x2"), py::arg("y2"),
             py::arg("label") = "", py::arg("value") = "")
        .def("__repr__", &Wire::to_string);

    // ── Circuit ─────────────────────────────────────────────────────
    py::class_<Circuit>(m, "Circuit")
        .def(py::init<>())
        .def_static("from_json", &Circuit::from_json,
             py::arg("json"),
             "Build a circuit from a JSON string array of components")
        .def("add_battery",  &Circuit::add_battery)
        .def("add_resistor", &Circuit::add_resistor)
        .def("add_wire",     &Circuit::add_wire)
        .def("size",         &Circuit::size)
        .def("to_string",    &Circuit::to_string)
        .def("__repr__",     &Circuit::to_string)
        .def("__len__",      &Circuit::size);

    // ── ComponentResult ─────────────────────────────────────────────
    py::class_<ComponentResult>(m, "ComponentResult")
        .def_readonly("label",               &ComponentResult::label)
        .def_readonly("type_name",           &ComponentResult::type_name)
        .def_readonly("resistance",          &ComponentResult::resistance)
        .def_readonly("voltage_drop",        &ComponentResult::voltage_drop)
        .def_readonly("current",             &ComponentResult::current)
        .def_readonly("power",               &ComponentResult::power)
        .def_readonly("emf",                 &ComponentResult::emf)
        .def_readonly("internal_resistance", &ComponentResult::internal_resistance)
        .def_readonly("terminal_voltage",    &ComponentResult::terminal_voltage)
        .def("__repr__", [](const ComponentResult& cr) {
            return "<ComponentResult " + cr.type_name + " [" + cr.label + "]>";
        });

    // ── CircuitSolution ─────────────────────────────────────────────
    py::class_<CircuitSolution>(m, "CircuitSolution")
        .def_readonly("node_voltages",     &CircuitSolution::node_voltages)
        .def_readonly("component_results", &CircuitSolution::component_results)
        .def_readonly("total_current",     &CircuitSolution::total_current);

    // ── CircuitSolver ───────────────────────────────────────────────
    py::class_<CircuitSolver>(m, "CircuitSolver")
        .def(py::init<const Circuit&>(), py::arg("circuit"))
        .def("solve", &CircuitSolver::solve,
             "Solve the circuit using Modified Nodal Analysis")
        .def_static("print_results", &CircuitSolver::print_results,
             py::arg("solution"),
             "Pretty-print a CircuitSolution to stdout");
}
