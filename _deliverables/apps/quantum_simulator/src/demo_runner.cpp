#include "demo_runner.hpp"

#include "quantum_algorithms.h"

#include <sstream>

namespace quantum_simulator_app {

using namespace quantum;
using namespace quantum::algorithms;

const std::vector<DemoDescriptor>& demo_list() {
    static const std::vector<DemoDescriptor> list = {
        {DemoId::Bell, "Bell State (2 qubits)"},
        {DemoId::Ghz, "GHZ State (3 qubits)"},
        {DemoId::DeutschJozsaConstant, "Deutsch-Jozsa: constant oracle"},
        {DemoId::DeutschJozsaBalanced, "Deutsch-Jozsa: balanced oracle"},
        {DemoId::Grover, "Grover's Search (4 qubits)"},
        {DemoId::ShorFactor15, "Shor's Algorithm: factor 15"},
        {DemoId::ShorFactor21, "Shor's Algorithm: factor 21"},
        {DemoId::ShorFactor33, "Shor's Algorithm: factor 33"},
        {DemoId::ShorFactor35, "Shor's Algorithm: factor 35"},
    };
    return list;
}

namespace {

DemoResult run_bell(std::mt19937_64&) {
    DemoResult result;
    result.title = "Bell State";
    QuantumState state(2);
    bell_state().apply(state);
    result.probabilities = state.outcome_probabilities();
    result.num_qubits_for_display = 2;
    result.log_lines.push_back("Prepared |00> and applied H(q0), then CNOT(q0 -> q1).");
    result.log_lines.push_back("Result: (|00> + |11>) / sqrt(2) -- a maximally entangled pair.");
    result.headline = "P(00) = P(11) = 50%, P(01) = P(10) = 0%";
    return result;
}

DemoResult run_ghz(std::mt19937_64&) {
    DemoResult result;
    result.title = "GHZ State";
    const int n = 3;
    QuantumState state(n);
    ghz_state(n).apply(state);
    result.probabilities = state.outcome_probabilities();
    result.num_qubits_for_display = n;
    result.log_lines.push_back("Prepared |000>, applied H(q0), then CNOT(q0->q1) and CNOT(q0->q2).");
    result.log_lines.push_back("Result: (|000> + |111>) / sqrt(2) -- 3-qubit entanglement.");
    result.headline = "P(000) = P(111) = 50%, everything else = 0%";
    return result;
}

DemoResult run_deutsch_jozsa(std::mt19937_64& rng, bool constant) {
    DemoResult result;
    result.title = constant ? "Deutsch-Jozsa (constant oracle)" : "Deutsch-Jozsa (balanced oracle)";
    const int n = 3;

    BooleanOracle oracle;
    if (constant) {
        oracle = [](std::uint64_t) { return 0; };
        result.log_lines.push_back("Oracle: f(x) = 0 for every input (constant).");
    } else {
        oracle = [](std::uint64_t x) {
            int bits = 0;
            while (x) { bits ^= static_cast<int>(x & 1); x >>= 1; }
            return bits;
        };
        result.log_lines.push_back("Oracle: f(x) = parity(x) (balanced -- 0 for half of all inputs, 1 for the other half).");
    }

    QuantumState state = deutsch_jozsa_state(n, oracle);
    result.probabilities = state.outcome_probabilities();
    result.num_qubits_for_display = n + 1; // includes the ancilla
    const bool is_constant = deutsch_jozsa(n, oracle, rng);
    result.log_lines.push_back("Prepared input register + ancilla, applied Hadamards, the oracle, then Hadamards again.");
    result.log_lines.push_back(std::string("Measured input register: ") + (is_constant ? "all zero" : "nonzero"));
    result.headline = is_constant ? "Conclusion: CONSTANT (1 query, vs up to 2^(n-1)+1 classically)"
                                  : "Conclusion: BALANCED (1 query, vs up to 2^(n-1)+1 classically)";
    return result;
}

DemoResult run_grover(std::mt19937_64& rng) {
    DemoResult result;
    result.title = "Grover's Search";
    const int n = 4;
    const std::uint64_t marked = 11;
    QuantumState state = grover_state(n, marked);
    result.probabilities = state.outcome_probabilities();
    result.num_qubits_for_display = n;

    const int iterations = optimal_grover_iterations(n);
    std::ostringstream line1;
    line1 << "Searching 2^" << n << " = 16 items for the marked item (index " << marked << ").";
    result.log_lines.push_back(line1.str());
    std::ostringstream line2;
    line2 << "Ran " << iterations << " Grover iteration(s) (oracle phase-flip + diffusion), the optimal count for N=16.";
    result.log_lines.push_back(line2.str());

    const std::uint64_t measured = state.measure_all(rng);
    std::ostringstream line3;
    line3 << "Measured index " << measured << " (" << (measured == marked ? "correct!" : "miss -- try again, it's probabilistic")
          << "), amplified from a classical 1/16 chance.";
    result.log_lines.push_back(line3.str());
    result.headline = "P(marked item) amplified to >90% after O(sqrt(N)) iterations";
    return result;
}

DemoResult run_shor(std::mt19937_64& rng, std::uint64_t n_to_factor) {
    DemoResult result;
    std::ostringstream title;
    title << "Shor's Algorithm: factor " << n_to_factor;
    result.title = title.str();

    const ShorResult shor_result = shor_factor(n_to_factor, rng, /*max_attempts=*/25);
    for (const auto& attempt : shor_result.attempts) {
        std::ostringstream line;
        if (attempt.a == 0) {
            line << "Classical shortcut: " << n_to_factor << " is a perfect power.";
        } else {
            line << "Attempt: chose a=" << attempt.a << ", gcd(a,N)=" << attempt.gcd_a_n;
            if (attempt.gcd_a_n == 1) {
                line << "; quantum order-finding (" << attempt.n_count_qubits << " counting + "
                     << attempt.n_work_qubits << " work qubits) measured " << attempt.measured_value
                     << " -> period r=" << attempt.period;
            }
        }
        if (attempt.succeeded) line << " -- SUCCESS, factor " << attempt.factor;
        else if (attempt.a != 0) line << " -- retry needed";
        result.log_lines.push_back(line.str());
    }

    if (shor_result.succeeded) {
        std::ostringstream headline;
        headline << "Factors found: " << shor_result.factor_a << " x " << shor_result.factor_b
                 << " = " << n_to_factor;
        result.headline = headline.str();
    } else {
        result.headline = "Failed to find factors within the attempt budget (rare -- try again).";
    }
    // Shor's doesn't have a single final "display state" in the same sense
    // as the other demos (the counting register is measured once per
    // attempt, not kept around) -- leave probabilities empty; the GUI shows
    // the log/headline instead of a bar chart for this demo.
    return result;
}

} // namespace

DemoResult run_demo(DemoId id, std::mt19937_64& rng) {
    switch (id) {
        case DemoId::Bell: return run_bell(rng);
        case DemoId::Ghz: return run_ghz(rng);
        case DemoId::DeutschJozsaConstant: return run_deutsch_jozsa(rng, true);
        case DemoId::DeutschJozsaBalanced: return run_deutsch_jozsa(rng, false);
        case DemoId::Grover: return run_grover(rng);
        case DemoId::ShorFactor15: return run_shor(rng, 15);
        case DemoId::ShorFactor21: return run_shor(rng, 21);
        case DemoId::ShorFactor33: return run_shor(rng, 33);
        case DemoId::ShorFactor35: return run_shor(rng, 35);
    }
    DemoResult fallback;
    fallback.title = "Unknown demo";
    return fallback;
}

} // namespace quantum_simulator_app
