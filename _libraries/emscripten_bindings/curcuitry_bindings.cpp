#include <emscripten/bind.h>
#include "curcuitry.h"

using namespace emscripten;
using namespace curcuitry;

// Wrapper to expose solve() returning a val (JS object)
val solve_circuit_json(const std::string& json_str) {
    Circuit circuit = Circuit::from_json(json_str);
    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    val result = val::object();

    // Node voltages
    val voltages = val::object();
    for (const auto& [node, voltage] : sol.node_voltages) {
        voltages.set(std::to_string(node), voltage);
    }
    result.set("node_voltages", voltages);

    // Component results
    val components = val::array();
    int idx = 0;
    for (const auto& cr : sol.component_results) {
        val comp = val::object();
        comp.set("label", cr.label);
        comp.set("type", cr.type_name);
        comp.set("resistance", cr.resistance);
        comp.set("voltage_drop", cr.voltage_drop);
        comp.set("current", cr.current);
        comp.set("power", cr.power);
        comp.set("emf", cr.emf);
        comp.set("internal_resistance", cr.internal_resistance);
        comp.set("terminal_voltage", cr.terminal_voltage);
        components.set(idx++, comp);
    }
    result.set("component_results", components);
    result.set("total_current", sol.total_current);

    return result;
}

EMSCRIPTEN_BINDINGS(curcuitry_module) {

    // ── ComponentType enum ─────────────────────────────────────
    enum_<ComponentType>("ComponentType")
        .value("BATTERY",  ComponentType::BATTERY)
        .value("RESISTOR", ComponentType::RESISTOR)
        .value("WIRE",     ComponentType::WIRE)
    ;

    // ── Point ──────────────────────────────────────────────────
    value_object<Point>("Point")
        .field("x", &Point::x)
        .field("y", &Point::y)
    ;

    // ── Battery ────────────────────────────────────────────────
    class_<Battery>("Battery")
        .constructor<double, double, double, double,
                     const std::string&, const std::string&,
                     double, double>()
        .function("voltage",             &Battery::voltage)
        .function("internal_resistance", &Battery::internal_resistance)
        .function("label",               &Battery::label)
        .function("to_string",           &Battery::to_string)
    ;

    // ── Resistor ───────────────────────────────────────────────
    class_<Resistor>("Resistor")
        .constructor<double, double, double, double,
                     const std::string&, const std::string&,
                     double>()
        .function("resistance", &Resistor::resistance)
        .function("label",      &Resistor::label)
        .function("to_string",  &Resistor::to_string)
    ;

    // ── Wire ───────────────────────────────────────────────────
    class_<Wire>("Wire")
        .constructor<double, double, double, double,
                     const std::string&, const std::string&>()
        .function("label",     &Wire::label)
        .function("to_string", &Wire::to_string)
    ;

    // ── Circuit ────────────────────────────────────────────────
    class_<Circuit>("Circuit")
        .constructor<>()
        .class_function("from_json", &Circuit::from_json)
        .function("add_battery",  &Circuit::add_battery)
        .function("add_resistor", &Circuit::add_resistor)
        .function("add_wire",     &Circuit::add_wire)
        .function("size",         &Circuit::size)
        .function("to_string",    &Circuit::to_string)
    ;

    // ── ComponentResult ────────────────────────────────────────
    value_object<ComponentResult>("ComponentResult")
        .field("label",               &ComponentResult::label)
        .field("type_name",           &ComponentResult::type_name)
        .field("resistance",          &ComponentResult::resistance)
        .field("voltage_drop",        &ComponentResult::voltage_drop)
        .field("current",             &ComponentResult::current)
        .field("power",               &ComponentResult::power)
        .field("emf",                 &ComponentResult::emf)
        .field("internal_resistance", &ComponentResult::internal_resistance)
        .field("terminal_voltage",    &ComponentResult::terminal_voltage)
    ;

    // ── CircuitSolution ────────────────────────────────────────
    value_object<CircuitSolution>("CircuitSolution")
        .field("component_results", &CircuitSolution::component_results)
        .field("total_current",     &CircuitSolution::total_current)
    ;

    register_vector<ComponentResult>("ComponentResultVector");

    // ── CircuitSolver ──────────────────────────────────────────
    class_<CircuitSolver>("CircuitSolver")
        .constructor<const Circuit&>()
        .function("solve", &CircuitSolver::solve)
    ;

    // ── Convenience function: JSON in → JS object out ──────────
    function("solve_circuit_json", &solve_circuit_json);
}
