#pragma once

#include "component.h"
#include <sstream>

namespace curcuitry {

/**
 * @brief A battery / voltage source component.
 *
 * Models an ideal voltage source with optional internal resistance.
 * - node1 (x1, y1) is the POSITIVE terminal.
 * - node2 (x2, y2) is the NEGATIVE terminal.
 * - voltage is the EMF in volts.
 * - internal_resistance is the series resistance in ohms (default 0).
 *
 * JSON fields:
 *   "type": "battery"
 *   "value": "10 V"
 *   "resistance": "0.1 Ω"   (optional, default 0)
 */
class Battery : public Component {
private:
    double voltage_;
    double internal_resistance_;

public:
    Battery()
        : Component(), voltage_(0.0), internal_resistance_(0.0) {}

    Battery(double x1, double y1, double x2, double y2,
            const std::string& label, const std::string& value,
            double voltage, double internal_resistance = 0.0)
        : Component(x1, y1, x2, y2, label, value),
          voltage_(voltage),
          internal_resistance_(internal_resistance) {}

    /**
     * @brief Construct a Battery from raw JSON field strings.
     * @param x1, y1 Positive terminal coordinates.
     * @param x2, y2 Negative terminal coordinates.
     * @param label Component label (e.g. "Vth").
     * @param value_str Raw value string (e.g. "10 V").
     * @param resistance_str Raw resistance string (e.g. "0.1 Ω").
     */
    static Battery from_fields(double x1, double y1, double x2, double y2,
                               const std::string& label,
                               const std::string& value_str,
                               const std::string& resistance_str = "") {
        double v = parse_numeric(value_str);
        double r = parse_numeric(resistance_str);
        return Battery(x1, y1, x2, y2, label, value_str, v, r);
    }

    // --- Component interface ---
    ComponentType type() const override { return ComponentType::BATTERY; }
    std::string type_name() const override { return "Battery"; }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Battery [" << label_ << "]"
            << " (" << node1_.x << "," << node1_.y << ")"
            << " -> (" << node2_.x << "," << node2_.y << ")"
            << " EMF=" << voltage_ << "V"
            << " Rint=" << internal_resistance_ << "Ω";
        return oss.str();
    }

    // --- Accessors ---
    double voltage() const { return voltage_; }
    double internal_resistance() const { return internal_resistance_; }

    void set_voltage(double v) { voltage_ = v; }
    void set_internal_resistance(double r) { internal_resistance_ = r; }
};

} // namespace curcuitry
