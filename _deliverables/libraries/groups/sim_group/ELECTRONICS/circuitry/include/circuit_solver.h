#ifndef CIRCUITRY_CIRCUIT_SOLVER_H
#define CIRCUITRY_CIRCUIT_SOLVER_H

#include "circuit.h"
#include "component.h"
#include "battery.h"
#include "resistor.h"
#include "wire.h"

#include "mytrix_eigen_compat.hpp"
// All matrix/vector types now use mytrix::Matrix, mytrix::Vector, etc.

#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace circuitry {

// ===================================================================
// Solution data structures
// ===================================================================

/**
 * @brief Per-component analysis result.
 */
struct ComponentResult {
    std::string label;
    std::string type_name;

    // Resistor fields
    double resistance     = 0.0;
    double voltage_drop   = 0.0;
    double current        = 0.0;
    double power          = 0.0;

    // Battery fields
    double emf                  = 0.0;
    double internal_resistance  = 0.0;
    double terminal_voltage     = 0.0;
};

/**
 * @brief Full solution of a circuit.
 */
struct CircuitSolution {
    /** Node index -> voltage (ground node is 0V). */
    std::map<int, double> node_voltages;

    /** Per-component detailed results. */
    std::vector<ComponentResult> component_results;
};

} // namespace circuitry

#endif // CIRCUITRY_CIRCUIT_SOLVER_H
