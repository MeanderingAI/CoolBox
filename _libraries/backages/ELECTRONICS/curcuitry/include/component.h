#pragma once

#include <string>
#include <utility>
#include <sstream>
#include <stdexcept>

namespace curcuitry {

/**
 * @brief 2D coordinate point for circuit layout.
 */
struct Point {
    double x;
    double y;

    Point() : x(0), y(0) {}
    Point(double x, double y) : x(x), y(y) {}

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Point& other) const {
        return !(*this == other);
    }

    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

/**
 * @brief Hash functor for Point, enabling use in unordered containers.
 */
struct PointHash {
    size_t operator()(const Point& p) const {
        auto h1 = std::hash<double>{}(p.x);
        auto h2 = std::hash<double>{}(p.y);
        return h1 ^ (h2 << 32);
    }
};

/**
 * @brief Enumeration of supported circuit component types.
 */
enum class ComponentType {
    BATTERY,
    RESISTOR,
    WIRE
};

/**
 * @brief Base class for all circuit components.
 *
 * Every component has two endpoint coordinates (node1, node2),
 * a label, and a raw value string. Derived classes add
 * type-specific parsed fields (voltage, resistance, etc.).
 */
class Component {
protected:
    Point node1_;
    Point node2_;
    std::string label_;
    std::string value_;

public:
    Component() = default;

    Component(double x1, double y1, double x2, double y2,
              const std::string& label, const std::string& value)
        : node1_(x1, y1), node2_(x2, y2), label_(label), value_(value) {}

    virtual ~Component() = default;

    /** @brief Returns the component type enum. */
    virtual ComponentType type() const = 0;

    /** @brief Returns a human-readable type name. */
    virtual std::string type_name() const = 0;

    /** @brief Returns a summary string for display. */
    virtual std::string to_string() const;

    // --- Accessors ---
    const Point& node1() const { return node1_; }
    const Point& node2() const { return node2_; }
    const std::string& label() const { return label_; }
    const std::string& value() const { return value_; }

    void set_node1(const Point& p) { node1_ = p; }
    void set_node2(const Point& p) { node2_ = p; }
    void set_label(const std::string& l) { label_ = l; }
    void set_value(const std::string& v) { value_ = v; }

    // --- Utility ---

    /**
     * @brief Parse a numeric value from a string like "10 V" or "2 Ω".
     * Strips non-numeric trailing characters.
     */
    static double parse_numeric(const std::string& s);
};

// ---------------------------------------------------------------------------
// Inline implementations
// ---------------------------------------------------------------------------

inline std::string Component::to_string() const {
    std::ostringstream oss;
    oss << type_name() << " [" << label_ << "]"
        << " (" << node1_.x << "," << node1_.y << ")"
        << " -> (" << node2_.x << "," << node2_.y << ")"
        << " value=\"" << value_ << "\"";
    return oss.str();
}

inline double Component::parse_numeric(const std::string& s) {
    if (s.empty()) return 0.0;
    // Find the longest leading substring that is a valid double
    std::string trimmed = s;
    // Remove leading/trailing whitespace
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) return 0.0;
    trimmed = trimmed.substr(start);

    try {
        size_t pos = 0;
        double val = std::stod(trimmed, &pos);
        return val;
    } catch (const std::exception&) {
        return 0.0;
    }
}

} // namespace curcuitry
