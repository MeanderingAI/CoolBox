#ifndef COOLBOX_APPS_QUANTUM_SIMULATOR_DEMO_RUNNER_HPP
#define COOLBOX_APPS_QUANTUM_SIMULATOR_DEMO_RUNNER_HPP

#include <random>
#include <string>
#include <vector>

namespace quantum_simulator_app {

enum class DemoId {
    Bell,
    Ghz,
    DeutschJozsaConstant,
    DeutschJozsaBalanced,
    Grover,
    ShorFactor15,
    ShorFactor21,
    ShorFactor33,
    ShorFactor35,
};

struct DemoDescriptor {
    DemoId id;
    const char* label; // short label for buttons/menus
};

// The full fixed list of demos offered by this app, in display order.
const std::vector<DemoDescriptor>& demo_list();

struct DemoResult {
    std::string title;
    std::string headline;               // one-line summary, e.g. "Factors: 3 x 5"
    std::vector<std::string> log_lines; // step-by-step narration
    std::vector<double> probabilities;  // outcome_probabilities() of the final state (empty if n/a)
    int num_qubits_for_display = 0;     // how many low-order bits of `probabilities`' index are meaningful labels
};

// Runs the requested demo end-to-end (building/measuring circuits as
// needed) and returns a narration + the resulting probability distribution,
// suitable for both console printing and GUI bar-chart rendering.
DemoResult run_demo(DemoId id, std::mt19937_64& rng);

} // namespace quantum_simulator_app

#endif // COOLBOX_APPS_QUANTUM_SIMULATOR_DEMO_RUNNER_HPP
