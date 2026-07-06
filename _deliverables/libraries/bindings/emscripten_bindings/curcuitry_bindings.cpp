#include <emscripten/bind.h>
#include "circuitry.h"

using namespace emscripten;
using namespace circuitry;

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
