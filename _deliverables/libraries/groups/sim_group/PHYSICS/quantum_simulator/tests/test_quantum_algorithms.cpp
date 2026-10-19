#include "tyst_framework.hpp"
#include "quantum_algorithms.h"

#include <algorithm>
#include <cmath>
#include <random>

using namespace quantum;
using namespace quantum::algorithms;

namespace {
constexpr double kEps = 1e-9;
bool approx(double a, double b, double eps = kEps) { return std::fabs(a - b) < eps; }
} // namespace

// ── Bell / GHZ ────────────────────────────────────────────────────────────────

TYST_TEST(QuantumAlgorithmsTests, BellStateIsMaximallyEntangled) {
    QuantumCircuit circuit = bell_state();
    QuantumState state(2);
    circuit.apply(state);
    const auto probs = state.outcome_probabilities();
    TYST_EXPECT_TRUE(approx(probs[0], 0.5)); // |00>
    TYST_EXPECT_TRUE(approx(probs[1], 0.0)); // |01>
    TYST_EXPECT_TRUE(approx(probs[2], 0.0)); // |10>
    TYST_EXPECT_TRUE(approx(probs[3], 0.5)); // |11>
}

TYST_TEST(QuantumAlgorithmsTests, BellStateMeasurementsAreCorrelated) {
    std::mt19937_64 rng(99);
    int matches = 0;
    const int trials = 500;
    for (int t = 0; t < trials; ++t) {
        QuantumState state(2);
        bell_state().apply(state);
        const int q0 = state.measure_qubit(0, rng);
        const int q1 = state.measure_qubit(1, rng);
        if (q0 == q1) ++matches;
    }
    TYST_EXPECT_EQ(matches, trials); // always perfectly correlated
}

TYST_TEST(QuantumAlgorithmsTests, GhzStateHasOnlyAllZeroAndAllOneOutcomes) {
    QuantumCircuit circuit = ghz_state(4);
    QuantumState state(4);
    circuit.apply(state);
    const auto probs = state.outcome_probabilities();
    TYST_EXPECT_TRUE(approx(probs[0], 0.5));             // |0000>
    TYST_EXPECT_TRUE(approx(probs[probs.size() - 1], 0.5)); // |1111>
    double other = 0.0;
    for (std::size_t i = 1; i + 1 < probs.size(); ++i) other += probs[i];
    TYST_EXPECT_TRUE(approx(other, 0.0));
}

TYST_TEST(QuantumAlgorithmsTests, GhzStateRejectsTooFewQubits) {
    TYST_EXPECT_THROW(ghz_state(1), std::invalid_argument);
}

// ── Deutsch-Jozsa ─────────────────────────────────────────────────────────────

TYST_TEST(QuantumAlgorithmsTests, DeutschJozsaIdentifiesConstantZeroFunction) {
    std::mt19937_64 rng(1);
    BooleanOracle constant_zero = [](std::uint64_t) { return 0; };
    TYST_EXPECT_TRUE(deutsch_jozsa(3, constant_zero, rng));
}

TYST_TEST(QuantumAlgorithmsTests, DeutschJozsaIdentifiesConstantOneFunction) {
    std::mt19937_64 rng(2);
    BooleanOracle constant_one = [](std::uint64_t) { return 1; };
    TYST_EXPECT_TRUE(deutsch_jozsa(3, constant_one, rng));
}

TYST_TEST(QuantumAlgorithmsTests, DeutschJozsaIdentifiesBalancedParityFunction) {
    std::mt19937_64 rng(3);
    // Parity of the input bits is a classic balanced function.
    BooleanOracle parity = [](std::uint64_t x) {
        int bits = 0;
        while (x) { bits ^= (x & 1); x >>= 1; }
        return bits;
    };
    TYST_EXPECT_FALSE(deutsch_jozsa(4, parity, rng));
}

TYST_TEST(QuantumAlgorithmsTests, DeutschJozsaIdentifiesBalancedMsbFunction) {
    std::mt19937_64 rng(4);
    const int n = 3;
    BooleanOracle msb = [n](std::uint64_t x) { return static_cast<int>((x >> (n - 1)) & 1); };
    TYST_EXPECT_FALSE(deutsch_jozsa(n, msb, rng));
}

TYST_TEST(QuantumAlgorithmsTests, DeutschJozsaStateProbabilitiesConfirmConstantVsBalanced) {
    BooleanOracle constant_zero = [](std::uint64_t) { return 0; };
    QuantumState constant_state = deutsch_jozsa_state(3, constant_zero);
    // All probability mass should be on input-register == 0 (ancilla can be
    // either value, so sum both ancilla possibilities for index 0 and 8).
    const auto probs = constant_state.outcome_probabilities();
    TYST_EXPECT_TRUE(approx(probs[0] + probs[8], 1.0));

    BooleanOracle parity = [](std::uint64_t x) {
        int bits = 0;
        while (x) { bits ^= (x & 1); x >>= 1; }
        return bits;
    };
    QuantumState balanced_state = deutsch_jozsa_state(3, parity);
    const auto balanced_probs = balanced_state.outcome_probabilities();
    TYST_EXPECT_TRUE(approx(balanced_probs[0] + balanced_probs[8], 0.0, 1e-6));
}

// ── Grover's search ───────────────────────────────────────────────────────────

TYST_TEST(QuantumAlgorithmsTests, OptimalGroverIterationsMatchesKnownFormula) {
    // N=16 -> floor(pi/4 * 4) = floor(3.14159) = 3
    TYST_EXPECT_EQ(optimal_grover_iterations(4), 3);
    // N=4 -> floor(pi/4*2) = floor(1.5708) = 1
    TYST_EXPECT_EQ(optimal_grover_iterations(2), 1);
}

TYST_TEST(QuantumAlgorithmsTests, GroverAmplifiesMarkedStateProbability) {
    for (int n = 2; n <= 5; ++n) {
        const std::uint64_t marked = (std::uint64_t{1} << n) / 3; // arbitrary in-range index
        QuantumState state = grover_state(n, marked);
        const auto probs = state.outcome_probabilities();
        TYST_EXPECT_TRUE(probs[marked] > 0.7);
    }
}

TYST_TEST(QuantumAlgorithmsTests, GroverSearchFindsMarkedIndexWithHighProbability) {
    std::mt19937_64 rng(55);
    const int n = 4;
    const std::uint64_t marked = 11;
    int hits = 0;
    const int trials = 200;
    for (int t = 0; t < trials; ++t) {
        if (grover_search(n, marked, rng) == marked) ++hits;
    }
    TYST_EXPECT_TRUE(static_cast<double>(hits) / trials > 0.7);
}

TYST_TEST(QuantumAlgorithmsTests, GroverRejectsOutOfRangeMarkedIndex) {
    TYST_EXPECT_THROW(grover_state(2, 10), std::invalid_argument);
}

// ── Shor's algorithm ──────────────────────────────────────────────────────────

TYST_TEST(QuantumAlgorithmsTests, ModPowMatchesNaiveExponentiation) {
    TYST_EXPECT_EQ(mod_pow(7, 0, 15), static_cast<std::uint64_t>(1));
    TYST_EXPECT_EQ(mod_pow(7, 1, 15), static_cast<std::uint64_t>(7));
    TYST_EXPECT_EQ(mod_pow(7, 2, 15), static_cast<std::uint64_t>(4));  // 49 mod 15 = 4
    TYST_EXPECT_EQ(mod_pow(2, 10, 1000), static_cast<std::uint64_t>(24)); // 1024 mod 1000
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorRejectsEvenNumbers) {
    std::mt19937_64 rng(1);
    TYST_EXPECT_THROW(shor_factor(20, rng), std::invalid_argument);
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorRejectsPrimes) {
    std::mt19937_64 rng(1);
    TYST_EXPECT_THROW(shor_factor(17, rng), std::invalid_argument);
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorRejectsTooSmallN) {
    std::mt19937_64 rng(1);
    TYST_EXPECT_THROW(shor_factor(9, rng), std::invalid_argument);
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorHandlesPerfectPowerClassically) {
    std::mt19937_64 rng(1);
    const ShorResult result = shor_factor(25, rng); // 5^2
    TYST_ASSERT_TRUE(result.succeeded);
    const std::uint64_t lo = std::min(result.factor_a, result.factor_b);
    const std::uint64_t hi = std::max(result.factor_a, result.factor_b);
    TYST_EXPECT_EQ(lo, static_cast<std::uint64_t>(5));
    TYST_EXPECT_EQ(hi, static_cast<std::uint64_t>(5));
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorFactors15) {
    std::mt19937_64 rng(42);
    const ShorResult result = shor_factor(15, rng, /*max_attempts=*/20);
    TYST_ASSERT_TRUE(result.succeeded);
    TYST_EXPECT_EQ(result.factor_a * result.factor_b, static_cast<std::uint64_t>(15));
    const std::uint64_t lo = std::min(result.factor_a, result.factor_b);
    const std::uint64_t hi = std::max(result.factor_a, result.factor_b);
    TYST_EXPECT_EQ(lo, static_cast<std::uint64_t>(3));
    TYST_EXPECT_EQ(hi, static_cast<std::uint64_t>(5));
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorFactors21) {
    std::mt19937_64 rng(7);
    const ShorResult result = shor_factor(21, rng, /*max_attempts=*/20);
    TYST_ASSERT_TRUE(result.succeeded);
    TYST_EXPECT_EQ(result.factor_a * result.factor_b, static_cast<std::uint64_t>(21));
}

TYST_TEST(QuantumAlgorithmsTests, ShorFactorRecordsAttemptHistory) {
    std::mt19937_64 rng(123);
    const ShorResult result = shor_factor(15, rng, /*max_attempts=*/20);
    TYST_ASSERT_TRUE(result.succeeded);
    TYST_EXPECT_TRUE(!result.attempts.empty());
    TYST_EXPECT_TRUE(result.attempts.back().succeeded);
}
