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
 * a label, and a raw value string. Derived classes add
 * ...existing code...
