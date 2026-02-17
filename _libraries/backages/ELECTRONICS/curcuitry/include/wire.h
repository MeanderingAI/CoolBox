#pragma once

#include "component.h"
#include <sstream>

namespace curcuitry {

/**
 * @brief A wire (ideal conductor) component.
 *
 * Models a zero-resistance connection between two points.
 * During circuit solving, wire endpoints are merged into
 * a single electrical node.
 *
 * JSON fields:
 *   "type": "wire"
 */
class Wire : public Component {
public:
    Wire() : Component() {}

    Wire(double x1, double y1, double x2, double y2,
         const std::string& label = "", const std::string& value = "")
        : Component(x1, y1, x2, y2, label, value) {}

    /**
     * @brief Construct a Wire from raw JSON field strings.
     */
    static Wire from_fields(double x1, double y1, double x2, double y2,
                            const std::string& label = "",
                            const std::string& value = "") {
        return Wire(x1, y1, x2, y2, label, value);
    }

    // --- Component interface ---
    ComponentType type() const override { return ComponentType::WIRE; }
    std::string type_name() const override { return "Wire"; }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Wire"
            << " (" << node1_.x << "," << node1_.y << ")"
            << " -> (" << node2_.x << "," << node2_.y << ")";
        if (!label_.empty()) oss << " [" << label_ << "]";
        return oss.str();
    }
};

} // namespace curcuitry
