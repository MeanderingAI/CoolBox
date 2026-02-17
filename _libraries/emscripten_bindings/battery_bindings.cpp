#include <emscripten/bind.h>
#include "battery.h"

using namespace emscripten;
using namespace battery;

EMSCRIPTEN_BINDINGS(battery_module) {

    // ── Chemistry enum ─────────────────────────────────────────
    enum_<Chemistry>("Chemistry")
        .value("LITHIUM_ION",              Chemistry::LITHIUM_ION)
        .value("LITHIUM_POLYMER",          Chemistry::LITHIUM_POLYMER)
        .value("LITHIUM_IRON_PHOSPHATE",   Chemistry::LITHIUM_IRON_PHOSPHATE)
        .value("NICKEL_METAL_HYDRIDE",     Chemistry::NICKEL_METAL_HYDRIDE)
        .value("LEAD_ACID",               Chemistry::LEAD_ACID)
        .value("ALKALINE",                Chemistry::ALKALINE)
    ;

    function("chemistry_name", &chemistry_name);

    // ── ChemistryDefaults ──────────────────────────────────────
    value_object<ChemistryDefaults>("ChemistryDefaults")
        .field("nominal_voltage",              &ChemistryDefaults::nominal_voltage)
        .field("max_voltage",                  &ChemistryDefaults::max_voltage)
        .field("min_voltage",                  &ChemistryDefaults::min_voltage)
        .field("typical_capacity",             &ChemistryDefaults::typical_capacity)
        .field("typical_internal_resistance",  &ChemistryDefaults::typical_internal_resistance)
    ;

    // ── Cell ───────────────────────────────────────────────────
    class_<Cell>("Cell")
        .constructor<const std::string&, Chemistry, double, double, double>()
        .constructor<const std::string&, Chemistry, double>()
        // SoC
        .function("soc",              &Cell::soc)
        .function("soc_percent",      &Cell::soc_percent)
        .function("is_depleted",      &Cell::is_depleted)
        .function("is_fully_charged", &Cell::is_fully_charged)
        // Voltage
        .function("open_circuit_voltage", &Cell::open_circuit_voltage)
        .function("terminal_voltage",     &Cell::terminal_voltage)
        .function("max_current",          &Cell::max_current)
        // Discharge / charge
        .function("discharge", &Cell::discharge)
        .function("charge",    &Cell::charge)
        // Accessors
        .function("label",               &Cell::label)
        .function("nominal_voltage",     &Cell::nominal_voltage)
        .function("max_voltage",         &Cell::max_voltage)
        .function("min_voltage",         &Cell::min_voltage)
        .function("capacity_ah",         &Cell::capacity_ah)
        .function("remaining_ah",        &Cell::remaining_ah)
        .function("internal_resistance", &Cell::internal_resistance)
        .function("temperature_c",       &Cell::temperature_c)
        .function("cycle_count",         &Cell::cycle_count)
        .function("set_temperature",         &Cell::set_temperature)
        .function("set_internal_resistance", &Cell::set_internal_resistance)
        .function("to_string", &Cell::to_string)
    ;

    // ── BatteryPack ────────────────────────────────────────────
    class_<BatteryPack>("BatteryPack")
        .constructor<const std::string&, int, int, const Cell&>()
        // Pack metrics
        .function("pack_voltage",              &BatteryPack::pack_voltage)
        .function("pack_terminal_voltage",     &BatteryPack::pack_terminal_voltage)
        .function("pack_capacity_ah",          &BatteryPack::pack_capacity_ah)
        .function("pack_energy_wh",            &BatteryPack::pack_energy_wh)
        .function("pack_internal_resistance",  &BatteryPack::pack_internal_resistance)
        .function("average_soc",  &BatteryPack::average_soc)
        .function("min_soc",      &BatteryPack::min_soc)
        .function("is_depleted",  &BatteryPack::is_depleted)
        // Discharge / charge
        .function("discharge", &BatteryPack::discharge)
        .function("charge",    &BatteryPack::charge)
        // Accessors
        .function("series_count",   &BatteryPack::series_count)
        .function("parallel_count", &BatteryPack::parallel_count)
        .function("total_cells",    &BatteryPack::total_cells)
        .function("label",          &BatteryPack::label)
        .function("to_string",      &BatteryPack::to_string)
        .function("print_status",   &BatteryPack::print_status)
    ;
}
