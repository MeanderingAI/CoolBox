#pragma once

#include "component.h"
#include <sstream>

namespace curcuitry {

/**
 * @brief A resistor component.
 *
 * Models a two-terminal resistor with a resistance value in ohms.
 *
 * JSON fields:
 *   "type": "resistor"
 *   "value": "2 Ω"
 */
class Resistor : public Component {
private:
    double resistance_;

public:
    Resistor()
        : Component(), resistance_(0.0) {}

    Resistor(double x1, double y1, double x2, double y2,
             const std::string& label, const std::string& value,
             double resistance)
        : Component(x1, y1, x2, y2, label, value),
          resistance_(resistance) {}

    /**
     * @brief Construct a Resistor from raw JSON field strings.
     * @param x1, y1 First terminal coordinates.
     * @param x2, y2 Second terminal coordinates.
     * @param label Component label (e.g. "Rth").
     * @param value_str Raw value string (e.g. "2 Ω").
     */
    static Resistor from_fields(double x1, double y1, double x2, double y2,
                                const std::string& label,
                                const std::string& value_str) {
        double r = parse_numeric(value_str);
        return Resistor(x1, y1, x2, y2, label, value_str, r);
    }

    // --- Component interface ---
    ComponentType type() const override { return ComponentType::RESISTOR; }
    std::string type_name() const override { return "Resistor"; }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Resistor [" << label_ << "]"
            << " (" << node1_.x << "," << node1_.y << ")"
            << " -> (" << node2_.x << "," << node2_.y << ")"
            << " R=" << resistance_ << "Ω";
        return oss.str();
    }

    // --- Accessors ---
    double resistance() const { return resistance_; }
    void set_resistance(double r) { resistance_ = r; }
};

} // namespace curcuitry
