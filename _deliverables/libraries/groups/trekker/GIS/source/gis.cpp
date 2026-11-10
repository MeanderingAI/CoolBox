#include "gis.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace trekker {
namespace gis {
namespace {

constexpr double pi = 3.14159265358979323846;

void check_coordinate(const Coordinate& point) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        (point.z && !std::isfinite(*point.z))) {
        throw std::invalid_argument("GIS coordinates must be finite");
    }
}

void check_geographic(const GeographicCoordinate& point) {
    if (!std::isfinite(point.longitude) || !std::isfinite(point.latitude) ||
        point.longitude < -180 || point.longitude > 180 ||
        point.latitude < -90 || point.latitude > 90) {
        throw std::invalid_argument("Longitude/latitude must be within [-180,180]/[-90,90]");
    }
}

void check_box(const BoundingBox& box) {
    check_coordinate({box.min_x, box.min_y, {}});
    check_coordinate({box.max_x, box.max_y, {}});
    if (box.min_x > box.max_x || box.min_y > box.max_y) {
        throw std::invalid_argument("Bounding box minima must not exceed maxima");
    }
}

void check_positions(const std::vector<Coordinate>& coordinates) {
    for (const auto& point : coordinates) {
        check_coordinate(point);
    }
}

void check_line(const LineString& line) {
    check_positions(line.coordinates);
    if (line.coordinates.size() < 2) {
        throw std::invalid_argument("LineString requires at least two positions");
    }
}

void check_ring(const std::vector<Coordinate>& ring) {
    check_positions(ring);
    if (ring.size() < 4 || ring.front().x != ring.back().x ||
        ring.front().y != ring.back().y || ring.front().z != ring.back().z) {
        throw std::invalid_argument("Polygon rings require at least four positions and exact closure");
    }
}

void check_polygon(const Polygon& polygon) {
    check_ring(polygon.exterior);
    for (const auto& hole : polygon.holes) {
        check_ring(hole);
    }
}

double checked_result(long double value) {
    const double result = static_cast<double>(value);
    if (!std::isfinite(result)) {
        throw std::overflow_error("GIS calculation exceeds the finite numeric range");
    }
    return result;
}

long double ring_area(const std::vector<Coordinate>& ring) {
    long double sum = 0;
    const long double x = ring.front().x;
    const long double y = ring.front().y;
    for (std::size_t i = 1; i < ring.size(); ++i) {
        sum += (static_cast<long double>(ring[i - 1].x) - x) *
                   (static_cast<long double>(ring[i].y) - y) -
               (static_cast<long double>(ring[i].x) - x) *
                   (static_cast<long double>(ring[i - 1].y) - y);
    }
    return std::fabs(sum) / 2;
}

enum class Location { outside, inside, boundary };

Location locate(const std::vector<Coordinate>& ring, const Coordinate& point) {
    bool inside = false;
    for (std::size_t i = 1; i < ring.size(); ++i) {
        const auto& a = ring[i - 1];
        const auto& b = ring[i];
        const long double dx = static_cast<long double>(b.x) - a.x;
        const long double dy = static_cast<long double>(b.y) - a.y;
        const long double px = static_cast<long double>(point.x) - a.x;
        const long double py = static_cast<long double>(point.y) - a.y;
        const long double cross = dx * py - dy * px;
        if (cross == 0 && point.x >= std::min(a.x, b.x) &&
            point.x <= std::max(a.x, b.x) && point.y >= std::min(a.y, b.y) &&
            point.y <= std::max(a.y, b.y)) {
            return Location::boundary;
        }
        if ((a.y > point.y) != (b.y > point.y) &&
            ((dy > 0 && cross > 0) || (dy < 0 && cross < 0))) {
            inside = !inside;
        }
    }
    return inside ? Location::inside : Location::outside;
}

template <typename Visitor>
void walk(const Geometry& geometry, const Visitor& visitor, unsigned depth = 0) {
    if (depth >= 64) {
        throw std::invalid_argument("GIS geometry nesting limit exceeded");
    }
    std::visit([&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Coordinate>) {
            check_coordinate(value);
            visitor(value);
        } else if constexpr (std::is_same_v<T, LineString> || std::is_same_v<T, MultiPoint>) {
            if constexpr (std::is_same_v<T, LineString>) {
                check_line(value);
            }
            check_positions(value.coordinates);
            for (const auto& point : value.coordinates) {
                visitor(point);
            }
        } else if constexpr (std::is_same_v<T, Polygon>) {
            check_polygon(value);
            for (const auto& point : value.exterior) {
                visitor(point);
            }
            for (const auto& hole : value.holes) {
                for (const auto& point : hole) {
                    visitor(point);
                }
            }
        } else if constexpr (std::is_same_v<T, MultiLineString>) {
            for (const auto& line : value.lines) {
                walk(Geometry{line, {}}, visitor, depth + 1);
            }
        } else if constexpr (std::is_same_v<T, MultiPolygon>) {
            for (const auto& polygon : value.polygons) {
                walk(Geometry{polygon, {}}, visitor, depth + 1);
            }
        } else {
            for (const auto& child : value.geometries) {
                walk(child, visitor, depth + 1);
            }
        }
    }, geometry.data);
}

} // namespace

Coordinate to_web_mercator(const GeographicCoordinate& coordinate) {
    check_geographic(coordinate);
    if (std::fabs(coordinate.latitude) > web_mercator_max_latitude) {
        throw std::invalid_argument("Latitude is outside the Web Mercator domain");
    }
    const double latitude = coordinate.latitude * pi / 180;
    // Keep rounding at the valid latitude endpoints inside the world extent.
    const double northing = std::clamp(std::asinh(std::tan(latitude)), -pi, pi);
    return {wgs84_radius_metres * (coordinate.longitude / 180 * pi),
            wgs84_radius_metres * northing, {}};
}

GeographicCoordinate from_web_mercator(const Coordinate& coordinate) {
    check_coordinate(coordinate);
    const double limit = pi * wgs84_radius_metres;
    if (std::fabs(coordinate.x) > limit || std::fabs(coordinate.y) > limit) {
        throw std::invalid_argument("Projected position is outside the Web Mercator world extent");
    }
    return {coordinate.x / limit * 180,
            std::clamp(std::atan(std::sinh(coordinate.y / wgs84_radius_metres)) * 180 / pi,
                       -web_mercator_max_latitude, web_mercator_max_latitude)};
}

double haversine_distance(const GeographicCoordinate& a, const GeographicCoordinate& b) {
    check_geographic(a);
    check_geographic(b);
    const double latitude_delta = (b.latitude - a.latitude) * pi / 180;
    const double longitude_delta = std::remainder(b.longitude - a.longitude, 360.0) * pi / 180;
    const double sin_latitude = std::sin(latitude_delta / 2);
    const double sin_longitude = std::sin(longitude_delta / 2);
    const double h = std::clamp(sin_latitude * sin_latitude +
        std::cos(a.latitude * pi / 180) * std::cos(b.latitude * pi / 180) *
        sin_longitude * sin_longitude, 0.0, 1.0);
    return 2 * mean_earth_radius_metres * std::atan2(std::sqrt(h), std::sqrt(1 - h));
}

bool BoundingBox::contains(const Coordinate& point) const {
    check_box(*this);
    check_coordinate(point);
    return point.x >= min_x && point.x <= max_x && point.y >= min_y && point.y <= max_y;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    check_box(*this);
    check_box(other);
    return min_x <= other.max_x && max_x >= other.min_x &&
           min_y <= other.max_y && max_y >= other.min_y;
}

void validate(const Geometry& geometry) {
    walk(geometry, [](const Coordinate&) {});
}

std::optional<BoundingBox> bounds(const Geometry& geometry) {
    std::optional<BoundingBox> result;
    walk(geometry, [&](const Coordinate& point) {
        if (!result) {
            result = BoundingBox{point.x, point.y, point.x, point.y};
        } else {
            result->min_x = std::min(result->min_x, point.x);
            result->min_y = std::min(result->min_y, point.y);
            result->max_x = std::max(result->max_x, point.x);
            result->max_y = std::max(result->max_y, point.y);
        }
    });
    return result;
}

double planar_length(const LineString& line) {
    check_line(line);
    long double result = 0;
    for (std::size_t i = 1; i < line.coordinates.size(); ++i) {
        result += std::hypot(
            static_cast<long double>(line.coordinates[i].x) - line.coordinates[i - 1].x,
            static_cast<long double>(line.coordinates[i].y) - line.coordinates[i - 1].y);
    }
    return checked_result(result);
}

double planar_area(const Polygon& polygon) {
    check_polygon(polygon);
    long double result = ring_area(polygon.exterior);
    for (const auto& hole : polygon.holes) {
        result -= ring_area(hole);
    }
    if (result < 0) {
        throw std::invalid_argument("Polygon holes exceed the exterior area");
    }
    return checked_result(result);
}

bool covers(const Polygon& polygon, const Coordinate& point) {
    check_polygon(polygon);
    check_coordinate(point);
    const auto exterior = locate(polygon.exterior, point);
    if (exterior == Location::outside) {
        return false;
    }
    for (const auto& hole : polygon.holes) {
        if (locate(hole, point) == Location::inside) {
            return false;
        }
    }
    return true;
}

BBTree::BBTree(const BBTree& other)
    : entries_(other.entries_), dirty_(!entries_.empty()) {}

BBTree::BBTree(BBTree&& other) noexcept {
    swap(other);
}

BBTree& BBTree::operator=(BBTree other) noexcept {
    swap(other);
    return *this;
}

void BBTree::swap(BBTree& other) noexcept {
    entries_.swap(other.entries_);
    nodes_.swap(other.nodes_);
    std::swap(height_, other.height_);
    std::swap(dirty_, other.dirty_);
}

void BBTree::insert(std::size_t id, const BoundingBox& box) {
    check_box(box);
    entries_.insert_or_assign(id, box);
    dirty_ = true;
}

bool BBTree::erase(std::size_t id) {
    if (entries_.erase(id) == 0) {
        return false;
    }
    dirty_ = true;
    return true;
}

void BBTree::rebuild() const {
    if (!dirty_) {
        return;
    }
    using Entry = std::pair<std::size_t, BoundingBox>;
    std::vector<Entry> entries(entries_.begin(), entries_.end());
    std::vector<Node> nodes;
    std::size_t height = 0;
    if (!entries.empty()) {
        if (entries.size() > nodes.max_size() / 2) {
            throw std::length_error("BBTree exceeds the supported node count");
        }
        nodes.reserve(entries.size() * 2 - 1);
        const auto build = [&](const auto& self, std::size_t first,
                               std::size_t last, std::size_t depth) -> std::size_t {
            BoundingBox box = entries[first].second;
            for (std::size_t i = first + 1; i < last; ++i) {
                box.min_x = std::min(box.min_x, entries[i].second.min_x);
                box.min_y = std::min(box.min_y, entries[i].second.min_y);
                box.max_x = std::max(box.max_x, entries[i].second.max_x);
                box.max_y = std::max(box.max_y, entries[i].second.max_y);
            }
            const std::size_t index = nodes.size();
            nodes.push_back(Node{box, entries[first].first, 0, 0, true});
            height = std::max(height, depth);
            if (last - first == 1) {
                return index;
            }
            // Halved spans and centers avoid overflow for finite world coordinates.
            const bool split_x = box.max_x / 2 - box.min_x / 2 >=
                                 box.max_y / 2 - box.min_y / 2;
            const auto center = [split_x](const BoundingBox& bounds) {
                return split_x ? bounds.min_x / 2 + bounds.max_x / 2
                               : bounds.min_y / 2 + bounds.max_y / 2;
            };
            const std::size_t middle = first + (last - first) / 2;
            std::nth_element(entries.begin() + first, entries.begin() + middle,
                             entries.begin() + last, [&](const Entry& a, const Entry& b) {
                const double a_center = center(a.second);
                const double b_center = center(b.second);
                return a_center != b_center ? a_center < b_center : a.first < b.first;
            });
            const auto left = self(self, first, middle, depth + 1);
            const auto right = self(self, middle, last, depth + 1);
            nodes[index].left = left;
            nodes[index].right = right;
            nodes[index].leaf = false;
            return index;
        };
        build(build, 0, entries.size(), 1);
    }
    nodes_.swap(nodes);
    height_ = height;
    dirty_ = false;
}

std::size_t BBTree::height() const {
    rebuild();
    return height_;
}

std::vector<std::size_t> BBTree::query(const BoundingBox& box) const {
    check_box(box);
    rebuild();
    std::vector<std::size_t> result;
    if (nodes_.empty()) {
        return result;
    }
    const auto search = [&](const auto& self, std::size_t index) -> void {
        const auto& node = nodes_[index];
        if (!node.box.intersects(box)) {
            return;
        }
        if (node.leaf) {
            result.push_back(node.id);
        } else {
            self(self, node.left);
            self(self, node.right);
        }
    };
    search(search, 0);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::size_t> BBTree::query(const Coordinate& point) const {
    check_coordinate(point);
    return query(BoundingBox{point.x, point.y, point.x, point.y});
}

} // namespace gis
} // namespace trekker
