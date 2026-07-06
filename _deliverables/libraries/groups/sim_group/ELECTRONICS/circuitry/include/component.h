#ifndef CIRCUITRY_COMPONENT_H
#define CIRCUITRY_COMPONENT_H

#include <string>
#include <utility>
#include <sstream>
#include <stdexcept>

namespace circuitry {

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
        return h1 ^ (h2 * 2654435761u);
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
 * a label, and a raw value string. Derived classes add additional fields.
 */

class Component {
public:
    Point node1_;
    Point node2_;
    std::string label_;
    std::string value_;

    Component()
        : node1_(), node2_(), label_(""), value_("") {}

    Component(double x1, double y1, double x2, double y2,
              const std::string& label, const std::string& value)
        : node1_(x1, y1), node2_(x2, y2), label_(label), value_(value) {}

    virtual ~Component() = default;

    virtual ComponentType type() const = 0;
    virtual std::string type_name() const = 0;
    virtual std::string to_string() const = 0;
};

} // namespace circuitry

#endif // CIRCUITRY_COMPONENT_H
