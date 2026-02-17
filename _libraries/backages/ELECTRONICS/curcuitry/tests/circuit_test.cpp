/**
 * @file circuit_test.cpp
 * @brief GTest suite for the curcuitry library.
 *
 * Tests cover:
 *   - Component construction and field parsing
 *   - Circuit JSON parsing
 *   - MNA solver correctness for series, parallel, and mixed circuits
 */

#include <gtest/gtest.h>
#include "curcuitry.h"

#include <cmath>
#include <string>

using namespace curcuitry;

static constexpr double TOL = 1e-4;

// ===================================================================
// Component unit tests
// ===================================================================

TEST(ComponentTest, ParseNumericBasic) {
    EXPECT_DOUBLE_EQ(Component::parse_numeric("10 V"), 10.0);
    EXPECT_DOUBLE_EQ(Component::parse_numeric("2 Ω"), 2.0);
    EXPECT_DOUBLE_EQ(Component::parse_numeric("0.1 Ω"), 0.1);
    EXPECT_DOUBLE_EQ(Component::parse_numeric(""), 0.0);
    EXPECT_DOUBLE_EQ(Component::parse_numeric("3.14"), 3.14);
}

// ===================================================================
// Battery tests
// ===================================================================

TEST(BatteryTest, FromFields) {
    Battery bat = Battery::from_fields(
        200, 200, 200, 300, "Vth", "10 V", "0.1 Ω");

    EXPECT_EQ(bat.type(), ComponentType::BATTERY);
    EXPECT_EQ(bat.type_name(), "Battery");
    EXPECT_EQ(bat.label(), "Vth");
    EXPECT_DOUBLE_EQ(bat.voltage(), 10.0);
    EXPECT_DOUBLE_EQ(bat.internal_resistance(), 0.1);
    EXPECT_EQ(bat.node1(), Point(200, 200));
    EXPECT_EQ(bat.node2(), Point(200, 300));
}

TEST(BatteryTest, ZeroInternalResistance) {
    Battery bat = Battery::from_fields(
        0, 0, 0, 100, "V1", "5 V", "");

    EXPECT_DOUBLE_EQ(bat.voltage(), 5.0);
    EXPECT_DOUBLE_EQ(bat.internal_resistance(), 0.0);
}

// ===================================================================
// Resistor tests
// ===================================================================

TEST(ResistorTest, FromFields) {
    Resistor res = Resistor::from_fields(
        300, 300, 500, 300, "Rth", "2 Ω");

    EXPECT_EQ(res.type(), ComponentType::RESISTOR);
    EXPECT_EQ(res.type_name(), "Resistor");
    EXPECT_EQ(res.label(), "Rth");
    EXPECT_DOUBLE_EQ(res.resistance(), 2.0);
    EXPECT_EQ(res.node1(), Point(300, 300));
    EXPECT_EQ(res.node2(), Point(500, 300));
}

// ===================================================================
// Wire tests
// ===================================================================

TEST(WireTest, FromFields) {
    Wire w = Wire::from_fields(200, 300, 300, 300);

    EXPECT_EQ(w.type(), ComponentType::WIRE);
    EXPECT_EQ(w.type_name(), "Wire");
    EXPECT_EQ(w.node1(), Point(200, 300));
    EXPECT_EQ(w.node2(), Point(300, 300));
}

// ===================================================================
// Circuit JSON parsing tests
// ===================================================================

TEST(CircuitTest, ParseTheveninJSON) {
    const std::string json = R"([
      {
        "type": "battery",
        "x1": 200, "y1": 200,
        "x2": 200, "y2": 300,
        "label": "Vth",
        "value": "10 V",
        "resistance": "0.1"
      },
      {
        "type": "wire",
        "x1": 200, "y1": 300,
        "x2": 300, "y2": 300,
        "label": "",
        "value": ""
      },
      {
        "type": "resistor",
        "x1": 300, "y1": 300,
        "x2": 500, "y2": 300,
        "label": "Rth",
        "value": "2"
      },
      {
        "type": "resistor",
        "x1": 500, "y1": 300,
        "x2": 500, "y2": 450,
        "label": "RL",
        "value": "8"
      },
      {
        "type": "wire",
        "x1": 500, "y1": 450,
        "x2": 200, "y2": 450,
        "label": "",
        "value": ""
      },
      {
        "type": "wire",
        "x1": 200, "y1": 450,
        "x2": 200, "y2": 200,
        "label": "",
        "value": ""
      }
    ])";

    Circuit circuit = Circuit::from_json(json);

    EXPECT_EQ(circuit.size(), 6u);
    EXPECT_EQ(circuit.batteries().size(), 1u);
    EXPECT_EQ(circuit.resistors().size(), 2u);
    EXPECT_EQ(circuit.wires().size(), 3u);

    EXPECT_DOUBLE_EQ(circuit.batteries()[0]->voltage(), 10.0);
    EXPECT_DOUBLE_EQ(circuit.batteries()[0]->internal_resistance(), 0.1);
    EXPECT_DOUBLE_EQ(circuit.resistors()[0]->resistance(), 2.0);
    EXPECT_DOUBLE_EQ(circuit.resistors()[1]->resistance(), 8.0);
}

TEST(CircuitTest, ParseEmptyArray) {
    Circuit circuit = Circuit::from_json("[]");
    EXPECT_EQ(circuit.size(), 0u);
}

TEST(CircuitTest, ParseUnknownTypeThrows) {
    const std::string json = R"([{"type":"capacitor","x1":0,"y1":0,"x2":1,"y2":1,"label":"C1","value":"1uF"}])";
    EXPECT_THROW(Circuit::from_json(json), std::runtime_error);
}

// ===================================================================
// CircuitSolver tests
// ===================================================================

/**
 * Thevenin equivalent circuit:
 *   Vth = 10 V, Rint = 0.1 Ω, Rth = 2 Ω, RL = 8 Ω
 *   Total R = 0.1 + 2 + 8 = 10.1 Ω
 *   I = 10 / 10.1 ≈ 0.990099 A
 *   V_RL = I * 8 ≈ 7.9208 V
 *   V_Rth = I * 2 ≈ 1.9802 V
 *   V_Rint = I * 0.1 ≈ 0.0990 V
 */
TEST(CircuitSolverTest, TheveninEquivalent) {
    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        200, 200, 200, 300, "Vth", "10 V", "0.1"));
    circuit.add_wire(Wire::from_fields(200, 300, 300, 300));
    circuit.add_resistor(Resistor::from_fields(
        300, 300, 500, 300, "Rth", "2"));
    circuit.add_resistor(Resistor::from_fields(
        500, 300, 500, 450, "RL", "8"));
    circuit.add_wire(Wire::from_fields(500, 450, 200, 450));
    circuit.add_wire(Wire::from_fields(200, 450, 200, 200));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    double expected_I = 10.0 / 10.1;

    // Find Rth result
    const ComponentResult* rth = nullptr;
    const ComponentResult* rl  = nullptr;
    const ComponentResult* vth = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "Rth") rth = &cr;
        if (cr.label == "RL")  rl  = &cr;
        if (cr.label == "Vth") vth = &cr;
    }

    ASSERT_NE(rth, nullptr);
    ASSERT_NE(rl, nullptr);
    ASSERT_NE(vth, nullptr);

    // Current through each series resistor should equal total current
    EXPECT_NEAR(std::abs(rth->current), expected_I, TOL);
    EXPECT_NEAR(std::abs(rl->current), expected_I, TOL);

    // Voltage drops
    EXPECT_NEAR(std::abs(rth->voltage_drop), expected_I * 2.0, TOL);
    EXPECT_NEAR(std::abs(rl->voltage_drop), expected_I * 8.0, TOL);

    // Battery terminal voltage < EMF due to internal resistance
    EXPECT_NEAR(vth->terminal_voltage, 10.0 - expected_I * 0.1, TOL);

    // Power dissipated in RL
    EXPECT_NEAR(rl->power, expected_I * expected_I * 8.0, TOL);
}

/**
 * Simple circuit: 10 V ideal battery + 10 Ω resistor
 * Expected: I = 1 A, V_R = 10 V, P = 10 W
 */
TEST(CircuitSolverTest, SimpleSeriesIdealBattery) {
    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        0, 0, 0, 100, "V1", "10 V", ""));
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 100, 100, "R1", "10"));
    circuit.add_wire(Wire::from_fields(100, 100, 100, 0));
    circuit.add_wire(Wire::from_fields(100, 0, 0, 0));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    const ComponentResult* r1 = nullptr;
    const ComponentResult* v1 = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "R1") r1 = &cr;
        if (cr.label == "V1") v1 = &cr;
    }

    ASSERT_NE(r1, nullptr);
    ASSERT_NE(v1, nullptr);

    EXPECT_NEAR(std::abs(r1->current), 1.0, TOL);
    EXPECT_NEAR(std::abs(r1->voltage_drop), 10.0, TOL);
    EXPECT_NEAR(r1->power, 10.0, TOL);
    EXPECT_NEAR(v1->terminal_voltage, 10.0, TOL);
}

/**
 * Two resistors in series: 12 V battery, R1=4Ω, R2=8Ω
 * Expected: I = 1 A, V_R1 = 4 V, V_R2 = 8 V
 */
TEST(CircuitSolverTest, TwoResistorsSeries) {
    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        0, 0, 0, 100, "V1", "12 V", ""));
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 50, 100, "R1", "4"));
    circuit.add_resistor(Resistor::from_fields(
        50, 100, 100, 100, "R2", "8"));
    circuit.add_wire(Wire::from_fields(100, 100, 100, 0));
    circuit.add_wire(Wire::from_fields(100, 0, 0, 0));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    const ComponentResult* r1 = nullptr;
    const ComponentResult* r2 = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "R1") r1 = &cr;
        if (cr.label == "R2") r2 = &cr;
    }

    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);

    EXPECT_NEAR(std::abs(r1->current), 1.0, TOL);
    EXPECT_NEAR(std::abs(r2->current), 1.0, TOL);
    EXPECT_NEAR(std::abs(r1->voltage_drop), 4.0, TOL);
    EXPECT_NEAR(std::abs(r2->voltage_drop), 8.0, TOL);
}

/**
 * Two resistors in parallel: 10 V battery, R1=10Ω, R2=10Ω
 * Expected: I_total = 2 A, I_R1 = I_R2 = 1 A, V = 10 V
 */
TEST(CircuitSolverTest, TwoResistorsParallel) {
    // Battery from (0,0) to (0,100)
    // R1 from (0,100) to (100,100) via wire to (100,0) back to (0,0)
    // R2 from (0,100) to (100,100) via a different spatial path
    // but electrically same nodes

    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        0, 0, 0, 100, "V1", "10 V", ""));

    // Both resistors connect the same two nodes: (0,100) and (200,100)
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 200, 100, "R1", "10"));
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 200, 100, "R2", "10"));

    // Wire from (200,100) back to (0,0) through (200,0)
    circuit.add_wire(Wire::from_fields(200, 100, 200, 0));
    circuit.add_wire(Wire::from_fields(200, 0, 0, 0));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    const ComponentResult* r1 = nullptr;
    const ComponentResult* r2 = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "R1") r1 = &cr;
        if (cr.label == "R2") r2 = &cr;
    }

    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);

    // Each resistor sees 10 V and carries 1 A
    EXPECT_NEAR(std::abs(r1->current), 1.0, TOL);
    EXPECT_NEAR(std::abs(r2->current), 1.0, TOL);
    EXPECT_NEAR(std::abs(r1->voltage_drop), 10.0, TOL);
    EXPECT_NEAR(std::abs(r2->voltage_drop), 10.0, TOL);

    // Total current from battery
    EXPECT_NEAR(sol.total_current, 2.0, TOL);
}

/**
 * Voltage divider: 12 V battery, R1=3Ω (top), R2=6Ω (bottom)
 * Expected: I = 12/9 ≈ 1.333 A
 *   V_R1 = 4 V, V_R2 = 8 V
 */
TEST(CircuitSolverTest, VoltageDivider) {
    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        0, 0, 0, 100, "V1", "12 V", ""));
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 100, 100, "R1", "3"));
    circuit.add_resistor(Resistor::from_fields(
        100, 100, 200, 100, "R2", "6"));
    circuit.add_wire(Wire::from_fields(200, 100, 200, 0));
    circuit.add_wire(Wire::from_fields(200, 0, 0, 0));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    const ComponentResult* r1 = nullptr;
    const ComponentResult* r2 = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "R1") r1 = &cr;
        if (cr.label == "R2") r2 = &cr;
    }

    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);

    double expected_I = 12.0 / 9.0;
    EXPECT_NEAR(std::abs(r1->current), expected_I, TOL);
    EXPECT_NEAR(std::abs(r1->voltage_drop), expected_I * 3.0, TOL);
    EXPECT_NEAR(std::abs(r2->voltage_drop), expected_I * 6.0, TOL);
}

/**
 * Battery with high internal resistance: V=5V, Rint=5Ω, RL=5Ω
 * Expected: I = 5/10 = 0.5 A, V_terminal = 5 - 0.5*5 = 2.5 V
 */
TEST(CircuitSolverTest, BatteryInternalResistance) {
    Circuit circuit;
    circuit.add_battery(Battery::from_fields(
        0, 0, 0, 100, "V1", "5 V", "5"));
    circuit.add_resistor(Resistor::from_fields(
        0, 100, 100, 100, "RL", "5"));
    circuit.add_wire(Wire::from_fields(100, 100, 100, 0));
    circuit.add_wire(Wire::from_fields(100, 0, 0, 0));

    CircuitSolver solver(circuit);
    CircuitSolution sol = solver.solve();

    const ComponentResult* rl = nullptr;
    const ComponentResult* v1 = nullptr;
    for (const auto& cr : sol.component_results) {
        if (cr.label == "RL") rl = &cr;
        if (cr.label == "V1") v1 = &cr;
    }

    ASSERT_NE(rl, nullptr);
    ASSERT_NE(v1, nullptr);

    EXPECT_NEAR(std::abs(rl->current), 0.5, TOL);
    EXPECT_NEAR(std::abs(rl->voltage_drop), 2.5, TOL);
    EXPECT_NEAR(v1->terminal_voltage, 2.5, TOL);
}

// ===================================================================
// UnionFind tests
// ===================================================================

TEST(UnionFindTest, MergesCorrectly) {
    UnionFind uf;
    Point a(0, 0), b(1, 1), c(2, 2);

    // Initially distinct
    EXPECT_EQ(uf.find(a), a);
    EXPECT_EQ(uf.find(b), b);

    // Merge a and b
    uf.unite(a, b);
    EXPECT_EQ(uf.find(a), uf.find(b));

    // c still distinct
    EXPECT_NE(uf.find(a), uf.find(c));

    // Merge b and c -> transitively a, b, c all same
    uf.unite(b, c);
    EXPECT_EQ(uf.find(a), uf.find(c));
}
