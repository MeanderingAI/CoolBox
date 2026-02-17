/**
 * @file battery_test.cpp
 * @brief GTest suite for the battery library.
 *
 * Tests cover:
 *   - Cell construction, SoC, voltage models
 *   - Discharge / charge simulation
 *   - BatteryPack series/parallel configurations
 */

#include <gtest/gtest.h>
#include "battery.h"

#include <cmath>

using namespace battery;

static constexpr double TOL = 1e-4;

// ===================================================================
// Cell construction tests
// ===================================================================

TEST(CellTest, DefaultLithiumIon) {
    Cell c("C1", Chemistry::LITHIUM_ION);
    EXPECT_EQ(c.label(), "C1");
    EXPECT_EQ(c.chemistry(), Chemistry::LITHIUM_ION);
    EXPECT_DOUBLE_EQ(c.nominal_voltage(), 3.7);
    EXPECT_DOUBLE_EQ(c.max_voltage(), 4.2);
    EXPECT_DOUBLE_EQ(c.min_voltage(), 3.0);
    EXPECT_DOUBLE_EQ(c.capacity_ah(), 2.5);
    EXPECT_NEAR(c.soc(), 1.0, TOL);
    EXPECT_FALSE(c.is_depleted());
    EXPECT_TRUE(c.is_fully_charged());
}

TEST(CellTest, CustomParameters) {
    Cell c("C2", Chemistry::LEAD_ACID, 10.0, 0.02, 0.5);
    EXPECT_EQ(c.chemistry(), Chemistry::LEAD_ACID);
    EXPECT_DOUBLE_EQ(c.capacity_ah(), 10.0);
    EXPECT_DOUBLE_EQ(c.internal_resistance(), 0.02);
    EXPECT_NEAR(c.soc(), 0.5, TOL);
    EXPECT_FALSE(c.is_fully_charged());
    EXPECT_FALSE(c.is_depleted());
}

TEST(CellTest, SoCClampedToRange) {
    Cell over("C3", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.5);
    EXPECT_NEAR(over.soc(), 1.0, TOL);

    Cell under("C4", Chemistry::LITHIUM_ION, 2.5, 0.05, -0.5);
    EXPECT_NEAR(under.soc(), 0.0, TOL);
}

// ===================================================================
// Voltage model tests
// ===================================================================

TEST(CellTest, OpenCircuitVoltageFullCharge) {
    Cell c("C1", Chemistry::LITHIUM_ION, 1.0);
    // SoC = 1.0 → V_oc = V_max = 4.2 V
    EXPECT_NEAR(c.open_circuit_voltage(), 4.2, TOL);
}

TEST(CellTest, OpenCircuitVoltageEmpty) {
    Cell c("C1", Chemistry::LITHIUM_ION, 0.0);
    // SoC = 0.0 → V_oc = V_min = 3.0 V
    EXPECT_NEAR(c.open_circuit_voltage(), 3.0, TOL);
}

TEST(CellTest, OpenCircuitVoltageHalf) {
    Cell c("C1", Chemistry::LITHIUM_ION, 0.5);
    // SoC = 0.5 → V_oc = 3.0 + 0.5*(4.2-3.0) = 3.6 V
    EXPECT_NEAR(c.open_circuit_voltage(), 3.6, TOL);
}

TEST(CellTest, TerminalVoltageUnderLoad) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    // V_oc = 4.2 V, I = 2 A, R = 0.05 Ω
    // V_terminal = 4.2 - 2*0.05 = 4.1 V
    EXPECT_NEAR(c.terminal_voltage(2.0), 4.1, TOL);
}

TEST(CellTest, MaxCurrent) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.1, 1.0);
    // V_oc = 4.2 V, V_min = 3.0 V, R = 0.1 Ω
    // I_max = (4.2 - 3.0) / 0.1 = 12 A
    EXPECT_NEAR(c.max_current(), 12.0, TOL);
}

// ===================================================================
// Discharge tests
// ===================================================================

TEST(CellTest, DischargeReducesSoC) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    double initial_soc = c.soc();

    // Discharge 1 A for 1 hour = 1 Ah
    c.discharge(1.0, 3600.0);

    EXPECT_LT(c.soc(), initial_soc);
    EXPECT_NEAR(c.remaining_ah(), 1.5, TOL);
    EXPECT_NEAR(c.soc(), 0.6, TOL);
}

TEST(CellTest, DischargeReturnsEnergy) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.0, 1.0);
    // 0 internal resistance for simpler calculation
    // V_oc starts at 4.2 V

    double energy = c.discharge(1.0, 3600.0);  // 1A for 1h
    // 1 Ah discharged, average voltage ~ (4.2 + 3.72) / 2 ≈ 3.96 V
    // Energy ≈ 1.0 * 3.96 = 3.96 Wh
    EXPECT_GT(energy, 0.0);
    EXPECT_NEAR(c.soc(), 0.6, TOL);
}

TEST(CellTest, DischargeCannotGoBelowZero) {
    Cell c("C1", Chemistry::LITHIUM_ION, 1.0, 0.05, 0.1);
    // Only 0.1 Ah remaining, try to discharge 1 Ah
    c.discharge(1.0, 3600.0);
    EXPECT_NEAR(c.remaining_ah(), 0.0, TOL);
    EXPECT_NEAR(c.soc(), 0.0, TOL);
}

TEST(CellTest, DischargeNegativeCurrentThrows) {
    Cell c("C1", Chemistry::LITHIUM_ION);
    EXPECT_THROW(c.discharge(-1.0, 3600.0), std::invalid_argument);
}

// ===================================================================
// Charge tests
// ===================================================================

TEST(CellTest, ChargeIncreasesSoC) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.05, 0.5);
    double initial_soc = c.soc();

    c.charge(1.0, 3600.0);  // 1 A for 1 hour

    EXPECT_GT(c.soc(), initial_soc);
    EXPECT_NEAR(c.remaining_ah(), 2.25, TOL);  // 1.25 + 1.0
}

TEST(CellTest, ChargeCannotExceedCapacity) {
    Cell c("C1", Chemistry::LITHIUM_ION, 2.5, 0.05, 0.9);
    // 0.25 Ah space, try to charge 1 Ah
    c.charge(1.0, 3600.0);
    EXPECT_NEAR(c.soc(), 1.0, TOL);
}

TEST(CellTest, ChargeNegativeCurrentThrows) {
    Cell c("C1", Chemistry::LITHIUM_ION);
    EXPECT_THROW(c.charge(-1.0, 3600.0), std::invalid_argument);
}

// ===================================================================
// Chemistry names
// ===================================================================

TEST(ChemistryTest, Names) {
    EXPECT_EQ(chemistry_name(Chemistry::LITHIUM_ION), "Li-ion");
    EXPECT_EQ(chemistry_name(Chemistry::LITHIUM_POLYMER), "LiPo");
    EXPECT_EQ(chemistry_name(Chemistry::LITHIUM_IRON_PHOSPHATE), "LiFePO4");
    EXPECT_EQ(chemistry_name(Chemistry::NICKEL_METAL_HYDRIDE), "NiMH");
    EXPECT_EQ(chemistry_name(Chemistry::LEAD_ACID), "Lead-Acid");
    EXPECT_EQ(chemistry_name(Chemistry::ALKALINE), "Alkaline");
}

// ===================================================================
// BatteryPack tests
// ===================================================================

TEST(BatteryPackTest, SingleCell1S1P) {
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack1", 1, 1, prototype);

    EXPECT_EQ(pack.total_cells(), 1);
    EXPECT_EQ(pack.series_count(), 1);
    EXPECT_EQ(pack.parallel_count(), 1);
    EXPECT_NEAR(pack.pack_voltage(), 4.2, TOL);
    EXPECT_NEAR(pack.pack_capacity_ah(), 2.5, TOL);
    EXPECT_NEAR(pack.pack_internal_resistance(), 0.05, TOL);
    EXPECT_NEAR(pack.average_soc(), 1.0, TOL);
}

TEST(BatteryPackTest, SeriesVoltageAdds) {
    // 3S1P Li-ion: 3 × 4.2V = 12.6V at full charge
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack3S", 3, 1, prototype);

    EXPECT_NEAR(pack.pack_voltage(), 12.6, TOL);
    EXPECT_NEAR(pack.pack_capacity_ah(), 2.5, TOL);
    EXPECT_NEAR(pack.pack_internal_resistance(), 0.15, TOL);  // 3 × 0.05
}

TEST(BatteryPackTest, ParallelCapacityAdds) {
    // 1S3P Li-ion: V same, capacity 3×
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack3P", 1, 3, prototype);

    EXPECT_NEAR(pack.pack_voltage(), 4.2, TOL);
    EXPECT_NEAR(pack.pack_capacity_ah(), 7.5, TOL);  // 3 × 2.5
    // Parallel R: 1/(3/0.05) = 0.05/3 ≈ 0.01667
    EXPECT_NEAR(pack.pack_internal_resistance(), 0.05 / 3.0, TOL);
}

TEST(BatteryPackTest, SeriesParallel4S2P) {
    // 4S2P Li-ion: V = 4 × 4.2 = 16.8, cap = 2 × 2.5 = 5.0
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack4S2P", 4, 2, prototype);

    EXPECT_EQ(pack.total_cells(), 8);
    EXPECT_NEAR(pack.pack_voltage(), 16.8, TOL);
    EXPECT_NEAR(pack.pack_capacity_ah(), 5.0, TOL);
    // R per stage = 0.05/2 = 0.025; 4 stages = 0.1
    EXPECT_NEAR(pack.pack_internal_resistance(), 0.1, TOL);
}

TEST(BatteryPackTest, TerminalVoltageUnderLoad) {
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.1, 1.0);
    BatteryPack pack("Pack1S1P", 1, 1, prototype);

    // V_oc = 4.2, I = 2A, R = 0.1 → V_t = 4.2 - 0.2 = 4.0
    EXPECT_NEAR(pack.pack_terminal_voltage(2.0), 4.0, TOL);
}

TEST(BatteryPackTest, DischargeReducesSoC) {
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack1S1P", 1, 1, prototype);

    double initial = pack.average_soc();
    pack.discharge(1.0, 3600.0);  // 1A for 1h
    EXPECT_LT(pack.average_soc(), initial);
}

TEST(BatteryPackTest, PackEnergyWh) {
    // 3S1P Li-ion: V_nom = 3×3.7 = 11.1V, cap = 2.5 Ah → 27.75 Wh
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack3S", 3, 1, prototype);

    EXPECT_NEAR(pack.pack_energy_wh(), 27.75, TOL);
}

TEST(BatteryPackTest, InvalidConfigThrows) {
    Cell prototype("proto", Chemistry::LITHIUM_ION);
    EXPECT_THROW(BatteryPack("bad", 0, 1, prototype), std::invalid_argument);
    EXPECT_THROW(BatteryPack("bad", 1, 0, prototype), std::invalid_argument);
}

TEST(BatteryPackTest, CellAccess) {
    Cell prototype("proto", Chemistry::LITHIUM_ION, 2.5, 0.05, 1.0);
    BatteryPack pack("Pack2S2P", 2, 2, prototype);

    // Verify cell labels
    EXPECT_EQ(pack.cell(0, 0).label(), "Pack2S2P_s0p0");
    EXPECT_EQ(pack.cell(0, 1).label(), "Pack2S2P_s0p1");
    EXPECT_EQ(pack.cell(1, 0).label(), "Pack2S2P_s1p0");
    EXPECT_EQ(pack.cell(1, 1).label(), "Pack2S2P_s1p1");
}
