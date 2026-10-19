#include "tyst_framework.hpp"
#include "quantum_simulator.h"

#include <cmath>
#include <complex>
#include <random>

using namespace quantum;

namespace {
constexpr double kEps = 1e-9;

bool approx(double a, double b, double eps = kEps) { return std::fabs(a - b) < eps; }
} // namespace

// ── Construction / basic state ───────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, NewStateStartsInAllZeroBasisState) {
    QuantumState state(3);
    TYST_EXPECT_EQ(state.num_qubits(), 3);
    TYST_EXPECT_EQ(state.dimension(), static_cast<std::size_t>(8));
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[0]), 1.0));
    for (std::size_t i = 1; i < state.dimension(); ++i) {
        TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[i]), 0.0));
    }
    TYST_EXPECT_TRUE(approx(state.total_probability(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, ConstructorRejectsNonPositiveOrTooLargeQubitCounts) {
    TYST_EXPECT_THROW(QuantumState(0), std::invalid_argument);
    TYST_EXPECT_THROW(QuantumState(-1), std::invalid_argument);
    TYST_EXPECT_THROW(QuantumState(QuantumState::kMaxQubits + 1), std::invalid_argument);
}

// ── Single-qubit gates ────────────────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, PauliXFlipsQubit) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::pauli_x());
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 1.0));
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[1]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, HadamardCreatesEqualSuperposition) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::hadamard());
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 0.5));
    TYST_EXPECT_TRUE(approx(state.total_probability(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, HadamardTwiceIsIdentity) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::hadamard());
    state.apply_single_qubit_gate(0, gates::hadamard());
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[0]), 1.0));
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[1]), 0.0));
}

TYST_TEST(QuantumSimulatorTests, PauliZLeavesZeroUnchangedAndFlipsOneSign) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::pauli_x());  // |1>
    state.apply_single_qubit_gate(0, gates::pauli_z());  // -> -|1>
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].real(), -1.0));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].imag(), 0.0));
}

TYST_TEST(QuantumSimulatorTests, PauliYMatchesIXZ) {
    // Y = i*X*Z, so applying Y to |0> should give i*|1> (since Z|0>=|0>, X|0>=|1>, i*|1> = i).
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::pauli_y());
    TYST_EXPECT_TRUE(approx(state.amplitudes()[0].real(), 0.0));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[0].imag(), 0.0));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].real(), 0.0));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].imag(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, SGateAppliesIPhaseToOne) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::pauli_x());
    state.apply_single_qubit_gate(0, gates::phase_s());
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].real(), 0.0));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[1].imag(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, SThenSDaggerIsIdentity) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::hadamard());
    state.apply_single_qubit_gate(0, gates::phase_s());
    state.apply_single_qubit_gate(0, gates::phase_s_dagger());
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 0.5));
    TYST_EXPECT_TRUE(approx(state.amplitudes()[0].imag(), 0.0, 1e-9));
}

TYST_TEST(QuantumSimulatorTests, TGateSquaredEqualsSGate) {
    QuantumState a(1), b(1);
    a.apply_single_qubit_gate(0, gates::pauli_x());
    a.apply_single_qubit_gate(0, gates::phase_t());
    a.apply_single_qubit_gate(0, gates::phase_t());

    b.apply_single_qubit_gate(0, gates::pauli_x());
    b.apply_single_qubit_gate(0, gates::phase_s());

    TYST_EXPECT_TRUE(approx(a.amplitudes()[1].real(), b.amplitudes()[1].real()));
    TYST_EXPECT_TRUE(approx(a.amplitudes()[1].imag(), b.amplitudes()[1].imag()));
}

TYST_TEST(QuantumSimulatorTests, RotationXByPiMatchesPauliXUpToGlobalPhase) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::rotation_x(M_PI));
    // Rx(pi) = -i*X, so |0> -> -i|1>.
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 1.0));
}

TYST_TEST(QuantumSimulatorTests, RotationZExpectationMatchesClosedForm) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::hadamard());
    const double theta = 0.7;
    state.apply_single_qubit_gate(0, gates::rotation_z(theta));
    // Rz doesn't change P(1) for a qubit starting in an equal superposition
    // (it's diagonal), so expectation_z should remain ~0.
    TYST_EXPECT_TRUE(approx(state.expectation_z(0), 0.0));
}

TYST_TEST(QuantumSimulatorTests, OutOfRangeQubitThrows) {
    QuantumState state(2);
    TYST_EXPECT_THROW(state.apply_single_qubit_gate(2, gates::pauli_x()), std::out_of_range);
    TYST_EXPECT_THROW(state.apply_single_qubit_gate(-1, gates::pauli_x()), std::out_of_range);
}

// ── Controlled / multi-controlled gates ──────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, CnotLeavesControlZeroUntouched) {
    QuantumState state(2);
    state.apply_controlled_gate(0, 1, gates::pauli_x());
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[0]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, CnotFlipsTargetWhenControlIsOne) {
    QuantumState state(2);
    state.apply_single_qubit_gate(0, gates::pauli_x()); // control = 1
    state.apply_controlled_gate(0, 1, gates::pauli_x());
    // state should now be |11> i.e. index 0b11 = 3
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[3]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, ControlEqualsTargetThrows) {
    QuantumState state(2);
    TYST_EXPECT_THROW(state.apply_controlled_gate(0, 0, gates::pauli_x()), std::invalid_argument);
}

TYST_TEST(QuantumSimulatorTests, ToffoliOnlyFlipsWhenBothControlsAreOne) {
    QuantumState state(3);
    state.apply_single_qubit_gate(0, gates::pauli_x());
    state.apply_single_qubit_gate(1, gates::pauli_x());
    state.apply_multi_controlled_gate({0, 1}, 2, gates::pauli_x());
    // |111> = index 0b111 = 7
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[7]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, ToffoliDoesNothingWithOnlyOneControlSet) {
    QuantumState state(3);
    state.apply_single_qubit_gate(0, gates::pauli_x()); // only control 0 set, control 1 stays |0>
    state.apply_multi_controlled_gate({0, 1}, 2, gates::pauli_x());
    // state should remain |001> = index 1 (target untouched)
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[1]), 1.0));
}

// ── SWAP ──────────────────────────────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, SwapExchangesQubitValues) {
    QuantumState state(2);
    state.apply_single_qubit_gate(0, gates::pauli_x()); // |01> (qubit0=1,qubit1=0)
    state.apply_swap(0, 1);
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 0.0));
    TYST_EXPECT_TRUE(approx(state.probability_one(1), 1.0));
}

// ── Permutation / diagonal-phase primitives ──────────────────────────────────

TYST_TEST(QuantumSimulatorTests, PermutationRelabelsBasisStates) {
    QuantumState state(2);
    state.apply_single_qubit_gate(0, gates::pauli_x()); // |01> -> index 1
    state.apply_permutation([](std::uint64_t i) { return i ^ 0b11ULL; }); // swap 1 <-> 2
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[2]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, DiagonalPhaseFlipsSignWithoutChangingProbabilities) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::hadamard());
    state.apply_diagonal_phase([](std::uint64_t i) { return (i == 1) ? M_PI : 0.0; });
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 0.5));
    TYST_EXPECT_TRUE(state.amplitudes()[1].real() < 0.0); // sign flipped
}

// ── Measurement ───────────────────────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, MeasureQubitCollapsesDeterministicStateConsistently) {
    QuantumState state(1);
    state.apply_single_qubit_gate(0, gates::pauli_x());
    std::mt19937_64 rng(42);
    const int outcome = state.measure_qubit(0, rng);
    TYST_EXPECT_EQ(outcome, 1);
    TYST_EXPECT_TRUE(approx(state.total_probability(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, MeasureQubitOnSuperpositionMatchesExpectedStatistics) {
    std::mt19937_64 rng(1234);
    int ones = 0;
    const int trials = 2000;
    for (int t = 0; t < trials; ++t) {
        QuantumState state(1);
        state.apply_single_qubit_gate(0, gates::hadamard());
        ones += state.measure_qubit(0, rng);
    }
    const double frequency = static_cast<double>(ones) / trials;
    TYST_EXPECT_TRUE(frequency > 0.4 && frequency < 0.6);
}

TYST_TEST(QuantumSimulatorTests, MeasureAllCollapsesToSingleBasisState) {
    QuantumState state(2);
    state.apply_single_qubit_gate(0, gates::hadamard());
    state.apply_single_qubit_gate(1, gates::hadamard());
    std::mt19937_64 rng(7);
    const std::uint64_t outcome = state.measure_all(rng);
    TYST_EXPECT_TRUE(outcome < 4);
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[outcome]), 1.0));
    TYST_EXPECT_TRUE(approx(state.total_probability(), 1.0));
}

TYST_TEST(QuantumSimulatorTests, OutcomeProbabilitiesSumToOne) {
    QuantumState state(3);
    state.apply_single_qubit_gate(0, gates::hadamard());
    state.apply_single_qubit_gate(1, gates::hadamard());
    state.apply_controlled_gate(0, 2, gates::pauli_x());
    const auto probs = state.outcome_probabilities();
    double total = 0.0;
    for (double p : probs) total += p;
    TYST_EXPECT_TRUE(approx(total, 1.0));
}

// ── QuantumCircuit ────────────────────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, CircuitAppliesGatesInOrder) {
    QuantumCircuit circuit(2);
    circuit.h(0).cnot(0, 1);
    QuantumState state(2);
    circuit.apply(state);
    // Bell state: P(00) = P(11) = 0.5, P(01) = P(10) = 0.
    const auto probs = state.outcome_probabilities();
    TYST_EXPECT_TRUE(approx(probs[0], 0.5));
    TYST_EXPECT_TRUE(approx(probs[1], 0.0));
    TYST_EXPECT_TRUE(approx(probs[2], 0.0));
    TYST_EXPECT_TRUE(approx(probs[3], 0.5));
}

TYST_TEST(QuantumSimulatorTests, CircuitQubitMismatchThrows) {
    QuantumCircuit circuit(2);
    circuit.h(0);
    QuantumState state(3);
    TYST_EXPECT_THROW(circuit.apply(state), std::invalid_argument);
}

TYST_TEST(QuantumSimulatorTests, CircuitDescribeListsGatesInOrder) {
    QuantumCircuit circuit(2);
    circuit.h(0).cnot(0, 1);
    const std::string text = circuit.describe();
    TYST_EXPECT_TRUE(text.find("H q0") != std::string::npos);
    TYST_EXPECT_TRUE(text.find("CNOT") != std::string::npos);
}

TYST_TEST(QuantumSimulatorTests, ToffoliBuilderMatchesManualMultiControlledGate) {
    QuantumCircuit circuit(3);
    circuit.x(0).x(1).toffoli(0, 1, 2);
    QuantumState state(3);
    circuit.apply(state);
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[7]), 1.0));
}

TYST_TEST(QuantumSimulatorTests, SwapBuilderExchangesQubits) {
    QuantumCircuit circuit(2);
    circuit.x(0).swap(0, 1);
    QuantumState state(2);
    circuit.apply(state);
    TYST_EXPECT_TRUE(approx(state.probability_one(1), 1.0));
    TYST_EXPECT_TRUE(approx(state.probability_one(0), 0.0));
}

// ── Quantum Fourier Transform ─────────────────────────────────────────────────

TYST_TEST(QuantumSimulatorTests, QftOnZeroStateIsUniformSuperposition) {
    QuantumState state(3);
    apply_qft(state, {0, 1, 2}, /*inverse=*/false);
    const auto probs = state.outcome_probabilities();
    for (double p : probs) TYST_EXPECT_TRUE(approx(p, 1.0 / 8.0));
}

TYST_TEST(QuantumSimulatorTests, QftThenInverseQftIsIdentity) {
    QuantumState state(3);
    state.apply_single_qubit_gate(0, gates::pauli_x());
    state.apply_single_qubit_gate(2, gates::pauli_x()); // arbitrary basis state |101>
    apply_qft(state, {0, 1, 2}, /*inverse=*/false);
    apply_qft(state, {0, 1, 2}, /*inverse=*/true);
    // Should return (up to floating point) to the original basis state index 5.
    TYST_EXPECT_TRUE(approx(std::norm(state.amplitudes()[5]), 1.0, 1e-6));
}

TYST_TEST(QuantumSimulatorTests, QftMatchesDiscreteFourierTransformDefinition) {
    // QFT|x> = (1/sqrt(N)) * sum_y e^{2*pi*i*x*y/N} |y>. Verify this exactly
    // for a small case (N=4, x=1) by comparing against the closed-form
    // amplitudes.
    QuantumState state(2);
    state.apply_single_qubit_gate(0, gates::pauli_x()); // |01> -> x = 1
    apply_qft(state, {0, 1}, /*inverse=*/false);

    const double n = 4.0;
    for (std::uint64_t y = 0; y < 4; ++y) {
        const double angle = 2.0 * M_PI * 1.0 * static_cast<double>(y) / n;
        const std::complex<double> expected = std::polar(1.0 / std::sqrt(n), angle);
        TYST_EXPECT_TRUE(approx(state.amplitudes()[y].real(), expected.real(), 1e-6));
        TYST_EXPECT_TRUE(approx(state.amplitudes()[y].imag(), expected.imag(), 1e-6));
    }
}
