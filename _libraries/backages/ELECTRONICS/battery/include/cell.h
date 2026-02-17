#pragma once

#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace battery {

/**
 * @brief Supported battery chemistry types.
 */
enum class Chemistry {
    LITHIUM_ION,        // Li-ion: 3.7V nominal, 4.2V max, 3.0V min
    LITHIUM_POLYMER,    // LiPo:  3.7V nominal, 4.2V max, 3.0V min
    LITHIUM_IRON_PHOSPHATE, // LiFePO4: 3.2V nominal, 3.65V max, 2.5V min
    NICKEL_METAL_HYDRIDE,   // NiMH: 1.2V nominal, 1.4V max, 1.0V min
    LEAD_ACID,          // PbA:  2.0V nominal, 2.4V max, 1.75V min
    ALKALINE            // 1.5V nominal, 1.6V max, 0.8V min
};

/**
 * @brief Returns the default voltage parameters for a chemistry.
 */
struct ChemistryDefaults {
    double nominal_voltage;   // V
    double max_voltage;       // V (fully charged)
    double min_voltage;       // V (cutoff)
    double typical_capacity;  // Ah
    double typical_internal_resistance; // Ω

    static ChemistryDefaults for_chemistry(Chemistry chem) {
        switch (chem) {
            case Chemistry::LITHIUM_ION:
                return {3.7, 4.2, 3.0, 2.5, 0.05};
            case Chemistry::LITHIUM_POLYMER:
                return {3.7, 4.2, 3.0, 2.0, 0.04};
            case Chemistry::LITHIUM_IRON_PHOSPHATE:
                return {3.2, 3.65, 2.5, 3.0, 0.03};
            case Chemistry::NICKEL_METAL_HYDRIDE:
                return {1.2, 1.4, 1.0, 2.0, 0.02};
            case Chemistry::LEAD_ACID:
                return {2.0, 2.4, 1.75, 7.0, 0.01};
            case Chemistry::ALKALINE:
                return {1.5, 1.6, 0.8, 2.5, 0.15};
            default:
                return {3.7, 4.2, 3.0, 2.5, 0.05};
        }
    }
};

/**
 * @brief Returns a human-readable name for a chemistry.
 */
inline std::string chemistry_name(Chemistry chem) {
    switch (chem) {
        case Chemistry::LITHIUM_ION:              return "Li-ion";
        case Chemistry::LITHIUM_POLYMER:          return "LiPo";
        case Chemistry::LITHIUM_IRON_PHOSPHATE:   return "LiFePO4";
        case Chemistry::NICKEL_METAL_HYDRIDE:     return "NiMH";
        case Chemistry::LEAD_ACID:                return "Lead-Acid";
        case Chemistry::ALKALINE:                 return "Alkaline";
        default:                                  return "Unknown";
    }
}

/**
 * @brief Models a single battery cell.
 *
 * Tracks voltage, capacity, state of charge (SoC), internal
 * resistance, and temperature. Provides methods to simulate
 * discharge/charge cycles and compute open-circuit voltage
 * from SoC using a linearised model.
 *
 * Open-circuit voltage model (linear approximation):
 *   V_oc(SoC) = V_min + SoC * (V_max - V_min)
 *
 * Terminal voltage under load:
 *   V_terminal = V_oc(SoC) - I * R_internal
 */
class Cell {
private:
    std::string label_;
    Chemistry   chemistry_;

    double nominal_voltage_;      // V
    double max_voltage_;          // V (fully charged OCV)
    double min_voltage_;          // V (cutoff OCV)

    double capacity_ah_;          // Amp-hours (rated)
    double remaining_ah_;         // Amp-hours remaining
    double internal_resistance_;  // Ω

    double temperature_c_;        // °C (ambient/cell temperature)
    int    cycle_count_;          // charge/discharge cycles

public:
    /**
     * @brief Construct a Cell with full specification.
     */
    Cell(const std::string& label,
         Chemistry chemistry,
         double capacity_ah,
         double internal_resistance,
         double initial_soc = 1.0)
        : label_(label),
          chemistry_(chemistry),
          temperature_c_(25.0),
          cycle_count_(0)
    {
        auto def = ChemistryDefaults::for_chemistry(chemistry);
        nominal_voltage_     = def.nominal_voltage;
        max_voltage_         = def.max_voltage;
        min_voltage_         = def.min_voltage;
        capacity_ah_         = capacity_ah;
        internal_resistance_ = internal_resistance;

        initial_soc = std::max(0.0, std::min(1.0, initial_soc));
        remaining_ah_ = capacity_ah_ * initial_soc;
    }

    /**
     * @brief Construct a Cell using chemistry defaults.
     */
    Cell(const std::string& label, Chemistry chemistry,
         double initial_soc = 1.0)
        : label_(label),
          chemistry_(chemistry),
          temperature_c_(25.0),
          cycle_count_(0)
    {
        auto def = ChemistryDefaults::for_chemistry(chemistry);
        nominal_voltage_     = def.nominal_voltage;
        max_voltage_         = def.max_voltage;
        min_voltage_         = def.min_voltage;
        capacity_ah_         = def.typical_capacity;
        internal_resistance_ = def.typical_internal_resistance;

        initial_soc = std::max(0.0, std::min(1.0, initial_soc));
        remaining_ah_ = capacity_ah_ * initial_soc;
    }

    // ---------------------------------------------------------------
    // State of Charge
    // ---------------------------------------------------------------

    /** @brief State of charge as a fraction [0.0, 1.0]. */
    double soc() const {
        return (capacity_ah_ > 0) ? remaining_ah_ / capacity_ah_ : 0.0;
    }

    /** @brief State of charge as a percentage [0, 100]. */
    double soc_percent() const { return soc() * 100.0; }

    /** @brief Is the cell considered depleted (below cutoff)? */
    bool is_depleted() const { return open_circuit_voltage() <= min_voltage_; }

    /** @brief Is the cell fully charged? */
    bool is_fully_charged() const { return soc() >= 0.9999; }

    // ---------------------------------------------------------------
    // Voltage models
    // ---------------------------------------------------------------

    /**
     * @brief Open-circuit voltage at current SoC (no-load).
     * Linear approximation: V_oc = V_min + SoC * (V_max - V_min)
     */
    double open_circuit_voltage() const {
        return min_voltage_ + soc() * (max_voltage_ - min_voltage_);
    }

    /**
     * @brief Terminal voltage under a given discharge current (A).
     * V_terminal = V_oc - I * R_internal
     * @param current_a  Discharge current in amperes (positive = discharge).
     */
    double terminal_voltage(double current_a) const {
        return open_circuit_voltage() - current_a * internal_resistance_;
    }

    /**
     * @brief Maximum deliverable current before hitting cutoff voltage.
     * I_max = (V_oc - V_min) / R_internal
     */
    double max_current() const {
        double v_oc = open_circuit_voltage();
        if (internal_resistance_ <= 0) return 1e9;  // effectively infinite
        return (v_oc - min_voltage_) / internal_resistance_;
    }

    // ---------------------------------------------------------------
    // Discharge / Charge simulation
    // ---------------------------------------------------------------

    /**
     * @brief Simulate discharging the cell at a given current for a duration.
     * @param current_a  Discharge current in amperes.
     * @param seconds    Duration in seconds.
     * @return Actual energy delivered in watt-hours.
     * @throws std::runtime_error if current exceeds max safe current.
     */
    double discharge(double current_a, double seconds) {
        if (current_a < 0)
            throw std::invalid_argument("Discharge current must be positive");
        if (is_depleted()) return 0.0;

        double hours = seconds / 3600.0;
        double ah_requested = current_a * hours;
        double ah_actual = std::min(ah_requested, remaining_ah_);

        // Average voltage during this discharge step
        double soc_before = soc();
        remaining_ah_ -= ah_actual;
        double soc_after = soc();
        double avg_soc = (soc_before + soc_after) / 2.0;
        double avg_voltage = min_voltage_ + avg_soc * (max_voltage_ - min_voltage_)
                           - current_a * internal_resistance_;
        avg_voltage = std::max(avg_voltage, 0.0);

        return ah_actual * avg_voltage;  // Wh
    }

    /**
     * @brief Simulate charging the cell at a given current for a duration.
     * @param current_a  Charge current in amperes (positive = charge).
     * @param seconds    Duration in seconds.
     * @return Actual energy absorbed in watt-hours.
     */
    double charge(double current_a, double seconds) {
        if (current_a < 0)
            throw std::invalid_argument("Charge current must be positive");
        if (is_fully_charged()) return 0.0;

        double hours = seconds / 3600.0;
        double ah_requested = current_a * hours;
        double ah_space = capacity_ah_ - remaining_ah_;
        double ah_actual = std::min(ah_requested, ah_space);

        double soc_before = soc();
        remaining_ah_ += ah_actual;
        double soc_after = soc();
        double avg_soc = (soc_before + soc_after) / 2.0;
        double avg_voltage = min_voltage_ + avg_soc * (max_voltage_ - min_voltage_)
                           + current_a * internal_resistance_;

        if (soc_after >= 0.9999 && soc_before < 0.9999) {
            ++cycle_count_;
        }

        return ah_actual * avg_voltage;  // Wh
    }

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    const std::string& label() const { return label_; }
    Chemistry chemistry() const { return chemistry_; }
    double nominal_voltage() const { return nominal_voltage_; }
    double max_voltage() const { return max_voltage_; }
    double min_voltage() const { return min_voltage_; }
    double capacity_ah() const { return capacity_ah_; }
    double remaining_ah() const { return remaining_ah_; }
    double internal_resistance() const { return internal_resistance_; }
    double temperature_c() const { return temperature_c_; }
    int cycle_count() const { return cycle_count_; }

    void set_temperature(double t) { temperature_c_ = t; }
    void set_internal_resistance(double r) { internal_resistance_ = r; }

    // ---------------------------------------------------------------
    // Display
    // ---------------------------------------------------------------

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Cell [" << label_ << "] "
            << chemistry_name(chemistry_)
            << " " << capacity_ah_ << "Ah"
            << " SoC=" << soc_percent() << "%"
            << " V_oc=" << open_circuit_voltage() << "V"
            << " R_int=" << internal_resistance_ << "Ω"
            << " cycles=" << cycle_count_;
        return oss.str();
    }
};

} // namespace battery
