#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "battery.h"

namespace py = pybind11;
using namespace battery;

PYBIND11_MODULE(battery_lib, m) {
    m.doc() = "Battery Cell & Pack Modeling – Python Bindings";

    // ── Chemistry enum ──────────────────────────────────────────────
    py::enum_<Chemistry>(m, "Chemistry")
        .value("LITHIUM_ION",              Chemistry::LITHIUM_ION)
        .value("LITHIUM_POLYMER",          Chemistry::LITHIUM_POLYMER)
        .value("LITHIUM_IRON_PHOSPHATE",   Chemistry::LITHIUM_IRON_PHOSPHATE)
        .value("NICKEL_METAL_HYDRIDE",     Chemistry::NICKEL_METAL_HYDRIDE)
        .value("LEAD_ACID",               Chemistry::LEAD_ACID)
        .value("ALKALINE",                Chemistry::ALKALINE);

    m.def("chemistry_name", &chemistry_name,
          py::arg("chemistry"),
          "Return a human-readable name for a battery chemistry");

    // ── ChemistryDefaults ───────────────────────────────────────────
    py::class_<ChemistryDefaults>(m, "ChemistryDefaults")
        .def_readonly("nominal_voltage",              &ChemistryDefaults::nominal_voltage)
        .def_readonly("max_voltage",                  &ChemistryDefaults::max_voltage)
        .def_readonly("min_voltage",                  &ChemistryDefaults::min_voltage)
        .def_readonly("typical_capacity",             &ChemistryDefaults::typical_capacity)
        .def_readonly("typical_internal_resistance",  &ChemistryDefaults::typical_internal_resistance)
        .def_static("for_chemistry", &ChemistryDefaults::for_chemistry,
             py::arg("chemistry"));

    // ── Cell ────────────────────────────────────────────────────────
    py::class_<Cell>(m, "Cell")
        .def(py::init<const std::string&, Chemistry, double, double, double>(),
             py::arg("label"),
             py::arg("chemistry"),
             py::arg("capacity_ah"),
             py::arg("internal_resistance"),
             py::arg("initial_soc") = 1.0)
        .def(py::init<const std::string&, Chemistry, double>(),
             py::arg("label"),
             py::arg("chemistry"),
             py::arg("initial_soc") = 1.0)
        // State of charge
        .def("soc",         &Cell::soc)
        .def("soc_percent", &Cell::soc_percent)
        .def("is_depleted",      &Cell::is_depleted)
        .def("is_fully_charged", &Cell::is_fully_charged)
        // Voltage
        .def("open_circuit_voltage", &Cell::open_circuit_voltage)
        .def("terminal_voltage",     &Cell::terminal_voltage,
             py::arg("current_a"))
        .def("max_current", &Cell::max_current)
        // Discharge / charge
        .def("discharge", &Cell::discharge,
             py::arg("current_a"), py::arg("seconds"),
             "Discharge cell; returns energy delivered in Wh")
        .def("charge", &Cell::charge,
             py::arg("current_a"), py::arg("seconds"),
             "Charge cell; returns energy absorbed in Wh")
        // Accessors
        .def("label",               &Cell::label)
        .def("chemistry",           &Cell::chemistry)
        .def("nominal_voltage",     &Cell::nominal_voltage)
        .def("max_voltage",         &Cell::max_voltage)
        .def("min_voltage",         &Cell::min_voltage)
        .def("capacity_ah",         &Cell::capacity_ah)
        .def("remaining_ah",        &Cell::remaining_ah)
        .def("internal_resistance", &Cell::internal_resistance)
        .def("temperature_c",       &Cell::temperature_c)
        .def("cycle_count",         &Cell::cycle_count)
        .def("set_temperature",          &Cell::set_temperature, py::arg("temp_c"))
        .def("set_internal_resistance",  &Cell::set_internal_resistance, py::arg("r"))
        .def("to_string", &Cell::to_string)
        .def("__repr__",  &Cell::to_string);

    // ── BatteryPack ─────────────────────────────────────────────────
    py::class_<BatteryPack>(m, "BatteryPack")
        .def(py::init<const std::string&, int, int, const Cell&>(),
             py::arg("label"),
             py::arg("series_count"),
             py::arg("parallel_count"),
             py::arg("prototype_cell"))
        // Pack metrics
        .def("pack_voltage",              &BatteryPack::pack_voltage)
        .def("pack_terminal_voltage",     &BatteryPack::pack_terminal_voltage,
             py::arg("pack_current_a"))
        .def("pack_capacity_ah",          &BatteryPack::pack_capacity_ah)
        .def("pack_energy_wh",            &BatteryPack::pack_energy_wh)
        .def("pack_internal_resistance",  &BatteryPack::pack_internal_resistance)
        .def("average_soc",  &BatteryPack::average_soc)
        .def("min_soc",      &BatteryPack::min_soc)
        .def("is_depleted",  &BatteryPack::is_depleted)
        // Discharge / charge
        .def("discharge", &BatteryPack::discharge,
             py::arg("current_a"), py::arg("seconds"),
             "Discharge pack; returns total energy delivered in Wh")
        .def("charge", &BatteryPack::charge,
             py::arg("current_a"), py::arg("seconds"),
             "Charge pack; returns total energy absorbed in Wh")
        // Cell access
        .def("cell", py::overload_cast<int, int>(&BatteryPack::cell),
             py::arg("s"), py::arg("p"),
             py::return_value_policy::reference_internal)
        .def("series_count",   &BatteryPack::series_count)
        .def("parallel_count", &BatteryPack::parallel_count)
        .def("total_cells",    &BatteryPack::total_cells)
        .def("label",          &BatteryPack::label)
        .def("to_string",      &BatteryPack::to_string)
        .def("print_status",   &BatteryPack::print_status)
        .def("__repr__",       &BatteryPack::to_string);
}
