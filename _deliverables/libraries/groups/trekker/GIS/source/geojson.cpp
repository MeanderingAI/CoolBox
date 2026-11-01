#include "gis.hpp"

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace trekker {
namespace gis {
namespace {

using dataformats::json::Array;
using dataformats::json::Object;
using dataformats::json::Parser;
using dataformats::json::Value;

Object object_value(const Value& value) {
    if (!value.is_object()) {
        throw std::invalid_argument("GeoJSON requires an object");
    }
    return value.as_object();
}

Array array_value(const Value& value) {
    if (!value.is_array()) {
        throw std::invalid_argument("GeoJSON requires an array");
    }
    return value.as_array();
}

Value required(const Object& object, const std::string& name) {
    if (!object.has(name)) {
        throw std::invalid_argument("GeoJSON is missing required member: " + name);
    }
    return object.get(name);
}

std::string type_name(const Object& object) {
    const auto type = required(object, "type");
    if (!type.is_string()) {
        throw std::invalid_argument("GeoJSON type must be a string");
    }
    return type.as_string();
}

double number_value(const Value& value) {
    if (!value.is_number() || !std::isfinite(value.as_number())) {
        throw std::invalid_argument("GeoJSON coordinates must be finite numbers");
    }
    return value.as_number();
}

void check_position(const Coordinate& coordinate) {
    if (!std::isfinite(coordinate.x) || !std::isfinite(coordinate.y) ||
        coordinate.x < -180 || coordinate.x > 180 ||
        coordinate.y < -90 || coordinate.y > 90 ||
        (coordinate.z && !std::isfinite(*coordinate.z))) {
        throw std::invalid_argument("GeoJSON positions must use finite WGS84 longitude/latitude");
    }
}

Coordinate decode_position(const Value& value) {
    const auto array = array_value(value);
    if (array.size() != 2 && array.size() != 3) {
        throw std::invalid_argument("GeoJSON positions require two or three ordinates");
    }
    Coordinate result{number_value(array.get(0)), number_value(array.get(1)), {}};
    if (array.size() == 3) {
        result.z = number_value(array.get(2));
    }
    check_position(result);
    return result;
}

std::vector<Coordinate> decode_positions(const Value& value) {
    std::vector<Coordinate> result;
    for (const auto& position : array_value(value)) {
        result.push_back(decode_position(position));
    }
    return result;
}

Polygon decode_polygon(const Value& value) {
    const auto rings = array_value(value);
    if (rings.empty()) {
        throw std::invalid_argument("GeoJSON Polygon requires an exterior ring");
    }
    Polygon result{decode_positions(rings.get(0)), {}};
    for (std::size_t i = 1; i < rings.size(); ++i) {
        result.holes.push_back(decode_positions(rings.get(i)));
    }
    return result;
}

void check_metadata(const Object& object) {
    if (object.has("crs")) {
        throw std::invalid_argument("Legacy GeoJSON crs is not supported; convert positions to WGS84");
    }
    if (object.has("bbox")) {
        const auto box = array_value(object.get("bbox"));
        if (box.size() != 4 && box.size() != 6) {
            throw std::invalid_argument("GeoJSON bbox requires four or six ordinates");
        }
        const std::size_t dimensions = box.size() / 2;
        check_position({number_value(box.get(0)), number_value(box.get(1)), {}});
        check_position({number_value(box.get(dimensions)), number_value(box.get(dimensions + 1)), {}});
        // Longitude bounds may cross the antimeridian, unlike planar BoundingBox.
        for (std::size_t i = 1; i < dimensions; ++i) {
            if (number_value(box.get(i)) > number_value(box.get(i + dimensions))) {
                throw std::invalid_argument("GeoJSON bbox minima must not exceed maxima");
            }
        }
    }
}

Object foreign_members(Object object, std::initializer_list<const char*> names) {
    check_metadata(object);
    for (const auto* name : names) {
        object.remove(name);
    }
    return object;
}

void check_foreign_members(const Object& object) {
    check_metadata(object);
    for (const auto* name : {"type", "coordinates", "geometries", "geometry", "properties", "features"}) {
        if (object.has(name)) {
            throw std::invalid_argument("GeoJSON foreign members must not override member: " + std::string(name));
        }
    }
}

Geometry decode_geometry(const Value& value, unsigned depth = 0) {
    if (depth >= 64) {
        throw std::invalid_argument("GeoJSON geometry nesting limit exceeded");
    }
    const auto object = object_value(value);
    const auto type = type_name(object);
    Geometry result;
    if (type == "GeometryCollection") {
        GeometryCollection collection;
        for (const auto& child : array_value(required(object, "geometries"))) {
            collection.geometries.push_back(decode_geometry(child, depth + 1));
        }
        result.data = std::move(collection);
        result.foreign_members = foreign_members(object, {"type", "geometries"});
    } else {
        const auto coordinates = required(object, "coordinates");
        if (type == "Point") {
            result.data = decode_position(coordinates);
        } else if (type == "LineString") {
            result.data = LineString{decode_positions(coordinates)};
        } else if (type == "Polygon") {
            result.data = decode_polygon(coordinates);
        } else if (type == "MultiPoint") {
            result.data = MultiPoint{decode_positions(coordinates)};
        } else if (type == "MultiLineString") {
            MultiLineString lines;
            for (const auto& line : array_value(coordinates)) {
                lines.lines.push_back(LineString{decode_positions(line)});
            }
            result.data = std::move(lines);
        } else if (type == "MultiPolygon") {
            MultiPolygon polygons;
            for (const auto& polygon : array_value(coordinates)) {
                polygons.polygons.push_back(decode_polygon(polygon));
            }
            result.data = std::move(polygons);
        } else {
            throw std::invalid_argument("Unsupported GeoJSON geometry type: " + type);
        }
        result.foreign_members = foreign_members(object, {"type", "coordinates"});
    }
    check_foreign_members(result.foreign_members);
    validate(result);
    return result;
}

void check_feature(const Feature& feature) {
    if (!feature.properties.is_null() && !feature.properties.is_object()) {
        throw std::invalid_argument("GeoJSON Feature properties must be an object or null");
    }
    if (feature.id && !feature.id->is_string() &&
        !(feature.id->is_number() && std::isfinite(feature.id->as_number()))) {
        throw std::invalid_argument("GeoJSON Feature id must be a string or finite number");
    }
    check_foreign_members(feature.foreign_members);
    if (feature.foreign_members.has("id")) {
        throw std::invalid_argument("GeoJSON foreign members must not override Feature id");
    }
}

Feature decode_feature(const Value& value) {
    const auto object = object_value(value);
    if (type_name(object) != "Feature") {
        throw std::invalid_argument("FeatureCollection may only contain Feature objects");
    }
    Feature result;
    const auto geometry = required(object, "geometry");
    if (!geometry.is_null()) {
        result.geometry = decode_geometry(geometry);
    }
    result.properties = required(object, "properties");
    if (object.has("id")) {
        result.id = object.get("id");
    }
    result.foreign_members = foreign_members(object, {"type", "geometry", "properties", "id"});
    check_feature(result);
    return result;
}

Value encode_position(const Coordinate& coordinate) {
    check_position(coordinate);
    Array result;
    result.push(coordinate.x);
    result.push(coordinate.y);
    if (coordinate.z) {
        result.push(*coordinate.z);
    }
    return result;
}

Value encode_positions(const std::vector<Coordinate>& coordinates) {
    Array result;
    for (const auto& coordinate : coordinates) {
        result.push(encode_position(coordinate));
    }
    return result;
}

Value encode_polygon(const Polygon& polygon) {
    Array result;
    result.push(encode_positions(polygon.exterior));
    for (const auto& hole : polygon.holes) {
        result.push(encode_positions(hole));
    }
    return result;
}

Value encode_geometry(const Geometry& geometry, unsigned depth = 0) {
    if (depth >= 64) {
        throw std::invalid_argument("GeoJSON geometry nesting limit exceeded");
    }
    validate(geometry);
    check_foreign_members(geometry.foreign_members);
    Object object = geometry.foreign_members;
    std::visit([&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Coordinate>) {
            object.set("type", "Point");
            object.set("coordinates", encode_position(value));
        } else if constexpr (std::is_same_v<T, LineString> || std::is_same_v<T, MultiPoint>) {
            object.set("type", std::is_same_v<T, LineString> ? "LineString" : "MultiPoint");
            object.set("coordinates", encode_positions(value.coordinates));
        } else if constexpr (std::is_same_v<T, Polygon>) {
            object.set("type", "Polygon");
            object.set("coordinates", encode_polygon(value));
        } else if constexpr (std::is_same_v<T, MultiLineString>) {
            object.set("type", "MultiLineString");
            Array coordinates;
            for (const auto& line : value.lines) {
                coordinates.push(encode_positions(line.coordinates));
            }
            object.set("coordinates", coordinates);
        } else if constexpr (std::is_same_v<T, MultiPolygon>) {
            object.set("type", "MultiPolygon");
            Array coordinates;
            for (const auto& polygon : value.polygons) {
                coordinates.push(encode_polygon(polygon));
            }
            object.set("coordinates", coordinates);
        } else {
            object.set("type", "GeometryCollection");
            Array geometries;
            for (const auto& child : value.geometries) {
                geometries.push(encode_geometry(child, depth + 1));
            }
            object.set("geometries", geometries);
        }
    }, geometry.data);
    return object;
}

Value encode_feature(const Feature& feature) {
    check_feature(feature);
    Object object = feature.foreign_members;
    object.set("type", "Feature");
    object.set("geometry", feature.geometry ? encode_geometry(*feature.geometry) : Value{});
    object.set("properties", feature.properties);
    if (feature.id) {
        object.set("id", *feature.id);
    }
    return object;
}

} // namespace

GeoJson read_geojson(const std::string& text) {
    const auto value = Parser::parse(text);
    const auto object = object_value(value);
    const auto type = type_name(object);
    if (type == "Feature") {
        return decode_feature(value);
    }
    if (type == "FeatureCollection") {
        FeatureCollection collection;
        for (const auto& feature : array_value(required(object, "features"))) {
            collection.features.push_back(decode_feature(feature));
        }
        collection.foreign_members = foreign_members(object, {"type", "features"});
        check_foreign_members(collection.foreign_members);
        return collection;
    }
    return decode_geometry(value);
}

std::string write_geojson(const GeoJson& document) {
    return std::visit([](const auto& value) -> Value {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Geometry>) {
            return encode_geometry(value);
        } else if constexpr (std::is_same_v<T, Feature>) {
            return encode_feature(value);
        } else {
            check_foreign_members(value.foreign_members);
            Object object = value.foreign_members;
            object.set("type", "FeatureCollection");
            Array features;
            for (const auto& feature : value.features) {
                features.push(encode_feature(feature));
            }
            object.set("features", features);
            return object;
        }
    }, document).to_string();
}

} // namespace gis
} // namespace trekker
