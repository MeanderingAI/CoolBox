#ifndef TREKKER_GIS_HPP
#define TREKKER_GIS_HPP

#include "json.h"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace trekker {
namespace gis {

struct Coordinate {
    double x = 0;
    double y = 0;
    std::optional<double> z;
};

struct GeographicCoordinate {
    double longitude = 0;
    double latitude = 0;
};

inline constexpr double wgs84_radius_metres = 6378137.0;
inline constexpr double mean_earth_radius_metres = 6371008.8;
inline constexpr double web_mercator_max_latitude = 85.0511287798066;

Coordinate to_web_mercator(const GeographicCoordinate& coordinate);
GeographicCoordinate from_web_mercator(const Coordinate& coordinate);
double haversine_distance(const GeographicCoordinate& a, const GeographicCoordinate& b);

struct BoundingBox {
    double min_x = 0;
    double min_y = 0;
    double max_x = 0;
    double max_y = 0;

    bool contains(const Coordinate& point) const;
    bool intersects(const BoundingBox& other) const;
};

struct LineString {
    std::vector<Coordinate> coordinates;
};

struct Polygon {
    std::vector<Coordinate> exterior;
    std::vector<std::vector<Coordinate>> holes;
};

struct MultiPoint {
    std::vector<Coordinate> coordinates;
};

struct MultiLineString {
    std::vector<LineString> lines;
};

struct MultiPolygon {
    std::vector<Polygon> polygons;
};

struct Geometry;
struct GeometryCollection {
    std::vector<Geometry> geometries;
};

struct Geometry {
    using Data = std::variant<Coordinate, LineString, Polygon, MultiPoint,
                              MultiLineString, MultiPolygon, GeometryCollection>;
    Data data;
    dataformats::json::Object foreign_members;
};

void validate(const Geometry& geometry);
std::optional<BoundingBox> bounds(const Geometry& geometry);
double planar_length(const LineString& line);
double planar_area(const Polygon& polygon);
bool covers(const Polygon& polygon, const Coordinate& point);

// Bounding-box candidate filtering; results are not exact geometry intersections.
class BBTree {
public:
    BBTree() = default;
    BBTree(const BBTree& other);
    BBTree(BBTree&& other) noexcept;
    BBTree& operator=(BBTree other) noexcept;

    void insert(std::size_t id, const BoundingBox& box);
    bool erase(std::size_t id);
    std::vector<std::size_t> query(const BoundingBox& box) const;
    std::vector<std::size_t> query(const Coordinate& point) const;
    std::size_t size() const { return entries_.size(); }
    std::size_t height() const;

private:
    struct Node {
        BoundingBox box;
        std::size_t id = 0;
        std::size_t left = 0;
        std::size_t right = 0;
        bool leaf = true;
    };

    void rebuild() const;
    void swap(BBTree& other) noexcept;
    std::map<std::size_t, BoundingBox> entries_;
    mutable std::vector<Node> nodes_;
    mutable std::size_t height_ = 0;
    mutable bool dirty_ = false;
};

class SpatialIndex {
public:
    void insert(std::size_t id, const BoundingBox& box) { tree_.insert(id, box); }
    bool erase(std::size_t id) { return tree_.erase(id); }
    std::vector<std::size_t> query(const BoundingBox& box) const { return tree_.query(box); }
    std::vector<std::size_t> query(const Coordinate& point) const { return tree_.query(point); }
    std::size_t size() const { return tree_.size(); }

private:
    BBTree tree_;
};

struct Feature {
    std::optional<Geometry> geometry;
    dataformats::json::Value properties;
    std::optional<dataformats::json::Value> id;
    dataformats::json::Object foreign_members;
};

struct FeatureCollection {
    std::vector<Feature> features;
    dataformats::json::Object foreign_members;
};

using GeoJson = std::variant<Geometry, Feature, FeatureCollection>;

GeoJson read_geojson(const std::string& text);
std::string write_geojson(const GeoJson& document);

} // namespace gis
} // namespace trekker

#endif
