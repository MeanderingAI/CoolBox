#include "gis.hpp"
#include "tyst_framework.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

using namespace trekker::gis;
using dataformats::json::Object;
using dataformats::json::Parser;
using dataformats::json::Value;

namespace {

Polygon square_with_hole() {
    return {{{0, 0, {}}, {10, 0, {}}, {10, 10, {}}, {0, 10, {}}, {0, 0, {}}},
            {{{2, 2, {}}, {2, 4, {}}, {4, 4, {}}, {4, 2, {}}, {2, 2, {}}}}};
}

} // namespace

TEST(GisCoordinates, MercatorKnownValues) {
    const auto origin = to_web_mercator({0, 0});
    EXPECT_NEAR(origin.x, 0, 1e-9);
    EXPECT_NEAR(origin.y, 0, 1e-9);
    const auto projected = to_web_mercator({180, 45});
    EXPECT_NEAR(projected.x, 20037508.342789244, 1e-6);
    EXPECT_NEAR(projected.y, 5621521.486192066, 1e-6);
}

TEST(GisCoordinates, MercatorRoundTripAndWorldEdges) {
    for (const GeographicCoordinate coordinate : {
             GeographicCoordinate{-73.9857, 40.7484}, {151.2093, -33.8688},
             {-180, -web_mercator_max_latitude}, {180, web_mercator_max_latitude}}) {
        const auto restored = from_web_mercator(to_web_mercator(coordinate));
        EXPECT_NEAR(restored.longitude, coordinate.longitude, 1e-10);
        EXPECT_NEAR(restored.latitude, coordinate.latitude, 1e-10);
        EXPECT_NO_THROW(to_web_mercator(restored));
    }
}

TEST(GisCoordinates, InvalidInputIsExplicit) {
    EXPECT_THROW(to_web_mercator(GeographicCoordinate{0, 90}), std::invalid_argument);
    EXPECT_THROW(to_web_mercator(GeographicCoordinate{181, 0}), std::invalid_argument);
    EXPECT_THROW(from_web_mercator(Coordinate{0, 30000000, {}}), std::invalid_argument);
    EXPECT_THROW(haversine_distance(GeographicCoordinate{0, -91}, GeographicCoordinate{}),
                 std::invalid_argument);
    EXPECT_THROW(to_web_mercator(GeographicCoordinate{std::numeric_limits<double>::quiet_NaN(), 0}),
                 std::invalid_argument);
}

TEST(GisCoordinates, HaversineKnownDistances) {
    EXPECT_NEAR(haversine_distance({0, 0}, {0, 0}), 0, 1e-9);
    EXPECT_NEAR(haversine_distance({0, 0}, {1, 0}), 111195.0802335329, 1e-6);
    EXPECT_NEAR(haversine_distance({179, 0}, {-179, 0}), 222390.1604670658, 1e-6);
    EXPECT_NEAR(haversine_distance({0, 0}, {180, 0}), 20015114.442035925, 1e-6);
    EXPECT_NEAR(haversine_distance({-73.9857, 40.7484}, {2.2945, 48.8584}),
                haversine_distance({2.2945, 48.8584}, {-73.9857, 40.7484}), 1e-8);
}

TEST(GisGeometry, BoundsIncludeEveryCollectionMember) {
    Geometry geometry{GeometryCollection{{
        Geometry{Coordinate{-4, 8, 50}, {}},
        Geometry{MultiLineString{{LineString{{{2, -3, {}}, {10, 6, {}}}}}}, {}},
        Geometry{MultiPolygon{{square_with_hole()}}, {}}
    }}, {}};
    const auto box = bounds(geometry);
    ASSERT_TRUE(box.has_value());
    EXPECT_EQ(box->min_x, -4);
    EXPECT_EQ(box->min_y, -3);
    EXPECT_EQ(box->max_x, 10);
    EXPECT_EQ(box->max_y, 10);
}

TEST(GisGeometry, EmptyCollectionsHaveNoBounds) {
    EXPECT_FALSE(bounds(Geometry{MultiPoint{}, {}}).has_value());
    EXPECT_FALSE(bounds(Geometry{MultiLineString{}, {}}).has_value());
    EXPECT_FALSE(bounds(Geometry{MultiPolygon{}, {}}).has_value());
    EXPECT_FALSE(bounds(Geometry{GeometryCollection{}, {}}).has_value());
}

TEST(GisGeometry, PlanarLengthAndArea) {
    EXPECT_NEAR(planar_length(LineString{{{0, 0, {}}, {3, 4, {}}, {6, 8, {}}}}), 10, 1e-12);
    auto polygon = square_with_hole();
    EXPECT_NEAR(planar_area(polygon), 96, 1e-12);
    std::reverse(polygon.exterior.begin(), polygon.exterior.end());
    std::reverse(polygon.holes.front().begin(), polygon.holes.front().end());
    EXPECT_NEAR(planar_area(polygon), 96, 1e-12);
}

TEST(GisGeometry, PolygonCoversInteriorAndBoundariesButNotHoles) {
    const auto polygon = square_with_hole();
    EXPECT_TRUE(covers(polygon, {1, 1, {}}));
    EXPECT_TRUE(covers(polygon, {0, 5, {}}));
    EXPECT_TRUE(covers(polygon, {10, 10, {}}));
    EXPECT_TRUE(covers(polygon, {2, 3, {}}));
    EXPECT_FALSE(covers(polygon, {3, 3, {}}));
    EXPECT_FALSE(covers(polygon, {-1, 1, {}}));
    EXPECT_FALSE(covers(polygon, {11, 5, {}}));
}

TEST(GisGeometry, LargeCoordinateOffsetsPreserveArea) {
    auto polygon = square_with_hole();
    for (auto& point : polygon.exterior) {
        point.x += 1e12;
        point.y += 1e12;
    }
    for (auto& point : polygon.holes.front()) {
        point.x += 1e12;
        point.y += 1e12;
    }
    EXPECT_NEAR(planar_area(polygon), 96, 1e-12);
}

TEST(GisGeometry, ConcavePolygonCoverage) {
    const Polygon polygon{{{0, 0, {}}, {6, 0, {}}, {6, 2, {}}, {2, 2, {}},
                           {2, 6, {}}, {0, 6, {}}, {0, 0, {}}}, {}};
    EXPECT_NEAR(planar_area(polygon), 20, 1e-12);
    EXPECT_TRUE(covers(polygon, {1, 5, {}}));
    EXPECT_TRUE(covers(polygon, {5, 1, {}}));
    EXPECT_TRUE(covers(polygon, {2, 2, {}}));
    EXPECT_FALSE(covers(polygon, {3, 3, {}}));
    EXPECT_FALSE(covers(polygon, {-1, 3, {}}));
}

TEST(GisGeometry, InvalidShapesAndOverflowAreRejected) {
    EXPECT_THROW(validate(Geometry{LineString{{{0, 0, {}}}}, {}}), std::invalid_argument);
    EXPECT_THROW(planar_area(Polygon{}), std::invalid_argument);
    auto polygon = square_with_hole();
    polygon.exterior.back().x = 1;
    EXPECT_THROW(planar_area(polygon), std::invalid_argument);
    polygon = square_with_hole();
    polygon.holes.front().back().z = 1;
    EXPECT_THROW(validate(Geometry{polygon, {}}), std::invalid_argument);
    EXPECT_THROW(bounds(Geometry{Coordinate{0, std::numeric_limits<double>::infinity(), {}}, {}}),
                 std::invalid_argument);
    const double largest = std::numeric_limits<double>::max();
    EXPECT_THROW(planar_length(LineString{{{-largest, 0, {}}, {largest, 0, {}}}}), std::overflow_error);
}

TEST(GisSpatial, BoxesIncludeTouchingEdges) {
    const BoundingBox box{0, 0, 10, 10};
    EXPECT_TRUE(box.contains({10, 0, {}}));
    EXPECT_TRUE(box.intersects({10, 10, 12, 12}));
    EXPECT_FALSE(box.intersects({11, 0, 12, 5}));
    EXPECT_THROW(box.intersects(BoundingBox{2, 0, 1, 1}), std::invalid_argument);
    EXPECT_THROW((BoundingBox{2, 0, 1, 1}).contains(Coordinate{}), std::invalid_argument);
}

TEST(GisSpatial, QueriesUpdatesAndDeletion) {
    SpatialIndex index;
    EXPECT_TRUE(index.query(Coordinate{}).empty());
    index.insert(9, {0, 0, 10, 10});
    index.insert(2, {5, 5, 15, 15});
    EXPECT_EQ(index.query(Coordinate{5, 5, {}}), (std::vector<std::size_t>{2, 9}));
    EXPECT_EQ(index.query(BoundingBox{10, 10, 11, 11}), (std::vector<std::size_t>{2, 9}));
    index.insert(9, {20, 20, 30, 30});
    EXPECT_EQ(index.size(), 2U);
    EXPECT_EQ(index.query(Coordinate{5, 5, {}}), (std::vector<std::size_t>{2}));
    EXPECT_TRUE(index.erase(2));
    EXPECT_FALSE(index.erase(2));
    EXPECT_THROW(index.insert(3, BoundingBox{1, 0, -1, 0}), std::invalid_argument);
    EXPECT_EQ(index.size(), 1U);
    EXPECT_THROW(index.query(Coordinate{std::numeric_limits<double>::infinity(), 0, {}}),
                 std::invalid_argument);
}

TEST(GisBBTree, EmptySingleLeafAndRemovalOfLastEntry) {
    BBTree tree;
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.height(), 0U);
    EXPECT_TRUE(tree.query(Coordinate{}).empty());
    EXPECT_FALSE(tree.erase(0));
    tree.insert(0, {1, 2, 1, 2});
    EXPECT_EQ(tree.height(), 1U);
    EXPECT_EQ(tree.query(Coordinate{1, 2, {}}), (std::vector<std::size_t>{0}));
    EXPECT_TRUE(tree.query(Coordinate{1, 3, {}}).empty());
    EXPECT_TRUE(tree.erase(0));
    EXPECT_EQ(tree.height(), 0U);
    EXPECT_TRUE(tree.query(BoundingBox{-10, -10, 10, 10}).empty());
    tree.insert(std::numeric_limits<std::size_t>::max(), {0, 0, 0, 0});
    EXPECT_EQ(tree.query(Coordinate{}),
              (std::vector<std::size_t>{std::numeric_limits<std::size_t>::max()}));
}

TEST(GisBBTree, BalancedWithOrderedAndIdenticalBoxes) {
    BBTree tree;
    for (std::size_t id = 0; id < 1024; ++id) {
        const double x = static_cast<double>(id) * 10;
        tree.insert(id, {x, 0, x + 1, 1});
    }
    EXPECT_EQ(tree.height(), 11U);
    EXPECT_EQ(tree.query(Coordinate{5000, 0, {}}), (std::vector<std::size_t>{500}));
    EXPECT_TRUE(tree.query(BoundingBox{-10, -10, -1, -1}).empty());
    for (std::size_t id = 0; id < 1024; ++id) {
        tree.insert(id, {0, 0, 1, 1});
    }
    EXPECT_EQ(tree.height(), 11U);
    auto matches = tree.query(Coordinate{1, 1, {}});
    ASSERT_EQ(matches.size(), 1024U);
    for (std::size_t id = 0; id < matches.size(); ++id) {
        EXPECT_EQ(matches[id], id);
    }
    for (std::size_t id = 0; id < 1023; ++id) {
        EXPECT_TRUE(tree.erase(id));
    }
    EXPECT_EQ(tree.height(), 1U);
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{1023}));
}

TEST(GisBBTree, InvalidUpdatesPreservePreviousMatches) {
    BBTree tree;
    tree.insert(1, {0, 0, 2, 2});
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{1}));
    EXPECT_THROW(tree.insert(1, BoundingBox{1, 0, -1, 1}), std::invalid_argument);
    EXPECT_THROW(tree.insert(2, BoundingBox{0, 0, std::numeric_limits<double>::infinity(), 1}),
                 std::invalid_argument);
    EXPECT_THROW(tree.query(BoundingBox{1, 0, 0, 1}), std::invalid_argument);
    EXPECT_THROW(tree.query(Coordinate{0, 0, std::numeric_limits<double>::quiet_NaN()}),
                 std::invalid_argument);
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{1}));
    EXPECT_EQ(tree.size(), 1U);
    tree.insert(1, {5, 5, 6, 6});
    EXPECT_FALSE(tree.erase(99));
    EXPECT_TRUE(tree.query(Coordinate{}).empty());
    EXPECT_EQ(tree.query(Coordinate{5, 5, {}}), (std::vector<std::size_t>{1}));
}

TEST(GisBBTree, ExtremeFiniteCoordinatesAndTouchingEdges) {
    const double limit = std::numeric_limits<double>::max();
    BBTree tree;
    tree.insert(1, {-limit, -limit, limit, limit});
    tree.insert(2, {limit, limit, limit, limit});
    tree.insert(3, {-limit, -limit, -limit, -limit});
    tree.insert(4, {0, 0, 0, 0});
    EXPECT_EQ(tree.height(), 3U);
    EXPECT_EQ(tree.query(Coordinate{limit, limit, {}}), (std::vector<std::size_t>{1, 2}));
    EXPECT_EQ(tree.query(Coordinate{-limit, -limit, {}}), (std::vector<std::size_t>{1, 3}));
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{1, 4}));
    EXPECT_EQ(tree.query(BoundingBox{-limit, -limit, limit, limit}),
              (std::vector<std::size_t>{1, 2, 3, 4}));
}

TEST(GisBBTree, CopiesHaveIndependentEntriesAndCaches) {
    BBTree original;
    original.insert(1, {0, 0, 1, 1});
    BBTree dirty_copy = original;
    original.insert(2, {0, 0, 1, 1});
    EXPECT_EQ(dirty_copy.query(Coordinate{}), (std::vector<std::size_t>{1}));
    EXPECT_EQ(original.query(Coordinate{}), (std::vector<std::size_t>{1, 2}));
    BBTree clean_copy = original;
    clean_copy.erase(1);
    EXPECT_EQ(clean_copy.query(Coordinate{}), (std::vector<std::size_t>{2}));
    EXPECT_EQ(original.query(Coordinate{}), (std::vector<std::size_t>{1, 2}));
    dirty_copy = clean_copy;
    EXPECT_EQ(dirty_copy.query(Coordinate{}), (std::vector<std::size_t>{2}));
}

TEST(GisBBTree, MovesPreserveCacheAndLeaveReusableEmptySource) {
    BBTree tree;
    tree.insert(1, {0, 0, 1, 1});
    EXPECT_EQ(tree.height(), 1U);
    BBTree moved(std::move(tree));
    EXPECT_EQ(moved.query(Coordinate{}), (std::vector<std::size_t>{1}));
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.height(), 0U);
    EXPECT_TRUE(tree.query(Coordinate{}).empty());
    tree.insert(2, {0, 0, 2, 2});
    moved = std::move(tree);
    EXPECT_EQ(moved.query(Coordinate{}), (std::vector<std::size_t>{2}));
    EXPECT_EQ(tree.height(), 0U);
    tree = moved;
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{2}));
    BBTree& alias = tree;
    tree = alias;
    EXPECT_EQ(tree.query(Coordinate{}), (std::vector<std::size_t>{2}));
}

TEST(GisBBTree, RandomizedUpdatesMatchBruteForce) {
    BBTree tree;
    SpatialIndex index;
    std::map<std::size_t, BoundingBox> reference;
    std::mt19937 generator(20261101);
    std::uniform_int_distribution<int> position(-1000, 1000);
    std::uniform_int_distribution<int> extent(0, 100);
    const auto random_box = [&]() {
        const double x = position(generator);
        const double y = position(generator);
        return BoundingBox{x, y, x + extent(generator), y + extent(generator)};
    };
    for (int iteration = 0; iteration < 3000; ++iteration) {
        const std::size_t id = generator() % 200;
        if (generator() % 4 == 0) {
            const bool erased = reference.erase(id) != 0;
            EXPECT_EQ(tree.erase(id), erased);
            EXPECT_EQ(index.erase(id), erased);
        } else {
            const auto box = random_box();
            reference.insert_or_assign(id, box);
            tree.insert(id, box);
            index.insert(id, box);
        }
        const auto box = random_box();
        const Coordinate point{box.min_x, box.min_y, {}};
        std::vector<std::size_t> box_matches;
        std::vector<std::size_t> point_matches;
        for (const auto& entry : reference) {
            if (entry.second.intersects(box)) {
                box_matches.push_back(entry.first);
            }
            if (entry.second.contains(point)) {
                point_matches.push_back(entry.first);
            }
        }
        EXPECT_EQ(tree.query(box), box_matches);
        EXPECT_EQ(index.query(box), box_matches);
        EXPECT_EQ(tree.query(point), point_matches);
        EXPECT_EQ(index.query(point), point_matches);
        EXPECT_EQ(tree.size(), reference.size());
        EXPECT_EQ(index.size(), reference.size());
        std::size_t height_bound = reference.empty() ? 0 : 1;
        std::size_t capacity = 1;
        while (capacity < reference.size()) {
            capacity *= 2;
            ++height_bound;
        }
        EXPECT_EQ(tree.height(), height_bound);
    }
}

TEST(GisGeoJson, AllSevenGeometryTypesRoundTrip) {
    const std::vector<std::string> documents{
        R"({"type":"Point","coordinates":[12.5,-45.25,123.456]})",
        R"({"type":"LineString","coordinates":[[0,0],[1,2],[3,4]]})",
        R"({"type":"Polygon","coordinates":[[[0,0],[10,0],[10,10],[0,10],[0,0]],[[2,2],[2,4],[4,4],[4,2],[2,2]]]})",
        R"({"type":"MultiPoint","coordinates":[[1,2],[3,4]]})",
        R"({"type":"MultiLineString","coordinates":[[[0,0],[1,1]],[[2,2],[3,3]]]})",
        R"({"type":"MultiPolygon","coordinates":[[[[0,0],[1,0],[1,1],[0,0]]],[[[2,2],[3,2],[3,3],[2,2]]]]})",
        R"({"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[1,2]},{"type":"GeometryCollection","geometries":[]}]})"
    };
    for (const auto& document : documents) {
        const auto parsed = read_geojson(document);
        ASSERT_TRUE(std::holds_alternative<Geometry>(parsed));
        const auto written = write_geojson(parsed);
        EXPECT_EQ(Parser::parse(written).to_string(), Parser::parse(document).to_string());
        EXPECT_EQ(write_geojson(read_geojson(written)), written);
    }
}

TEST(GisGeoJson, FeaturePropertiesIdsAndForeignMembersSurvive) {
    const std::string document =
        R"({"type":"Feature","id":"site-1","geometry":{"type":"Point","coordinates":[12.345678901234567,45.25,100],"bbox":[12,45,13,46],"source":"survey"},"properties":{"name":"caf\u00e9","nested":{"active":true,"values":[1,null,"x"]}},"extra":{"note":"keep"}})";
    const auto parsed = read_geojson(document);
    ASSERT_TRUE(std::holds_alternative<Feature>(parsed));
    const auto& feature = std::get<Feature>(parsed);
    ASSERT_TRUE(feature.geometry.has_value());
    ASSERT_TRUE(feature.id.has_value());
    EXPECT_EQ(feature.id->as_string(), "site-1");
    EXPECT_EQ(std::get<Coordinate>(feature.geometry->data).x, 12.345678901234567);
    EXPECT_EQ(std::get<Coordinate>(feature.geometry->data).z.value(), 100);
    EXPECT_EQ(Parser::parse(write_geojson(parsed)).to_string(), Parser::parse(document).to_string());
}

TEST(GisGeoJson, FeatureCollectionAndNullGeometry) {
    const std::string document =
        R"({"type":"FeatureCollection","bbox":[170,-10,-170,10],"source":"example","features":[{"type":"Feature","id":42,"geometry":null,"properties":null},{"type":"Feature","geometry":{"type":"Point","coordinates":[1,2]},"properties":{}}]})";
    const auto parsed = read_geojson(document);
    ASSERT_TRUE(std::holds_alternative<FeatureCollection>(parsed));
    const auto& collection = std::get<FeatureCollection>(parsed);
    ASSERT_EQ(collection.features.size(), 2U);
    EXPECT_FALSE(collection.features[0].geometry.has_value());
    EXPECT_TRUE(collection.features[0].properties.is_null());
    EXPECT_EQ(collection.features[0].id->as_number(), 42);
    EXPECT_EQ(Parser::parse(write_geojson(parsed)).to_string(), Parser::parse(document).to_string());
}

TEST(GisGeoJson, ThreeDimensionalBboxAndGeometryForeignId) {
    const std::string document =
        R"({"type":"Point","coordinates":[1,2,3],"id":"survey","bbox":[0,1,-10,2,3,20]})";
    EXPECT_EQ(Parser::parse(write_geojson(read_geojson(document))).to_string(),
              Parser::parse(document).to_string());
    EXPECT_THROW(read_geojson(
        R"({"type":"Point","coordinates":[1,2,3],"bbox":[0,1,10,2,3,0]})"),
        std::invalid_argument);
}

TEST(GisGeoJson, EmptyCollectionsRoundTrip) {
    for (const auto* text : {
        R"({"type":"MultiPoint","coordinates":[]})",
        R"({"type":"MultiLineString","coordinates":[]})",
        R"({"type":"MultiPolygon","coordinates":[]})",
        R"({"type":"GeometryCollection","geometries":[]})",
        R"({"type":"FeatureCollection","features":[]})"}) {
        EXPECT_EQ(Parser::parse(write_geojson(read_geojson(text))).to_string(),
                  Parser::parse(text).to_string());
    }
}

TEST(GisGeoJson, MalformedDocumentsAreRejected) {
    for (const auto* text : {
        R"(null)", R"([])", R"({})", R"({"type":1})",
        R"({"type":"Point","coordinates":[1]})",
        R"({"type":"Point","coordinates":[1,2,3,4]})",
        R"({"type":"Point","coordinates":["1",2]})",
        R"({"type":"Point","coordinates":[1,91]})",
        R"({"type":"Point","coordinates":[181,0]})",
        R"({"type":"Point","coordinates":[1,2],"bbox":[0,0,1]})",
        R"({"type":"Point","coordinates":[1,2],"bbox":[0,3,1,2]})",
        R"({"type":"Point","coordinates":[1,2],"crs":{"type":"name"}})",
        R"({"type":"LineString","coordinates":[[0,0]]})",
        R"({"type":"Polygon","coordinates":[]})",
        R"({"type":"Polygon","coordinates":[[[0,0],[1,0],[1,1],[0,1]]]})",
        R"({"type":"Unsupported","coordinates":[]})",
        R"({"type":"Feature","properties":{}})",
        R"({"type":"Feature","geometry":null})",
        R"({"type":"Feature","geometry":null,"properties":[]})",
        R"({"type":"Feature","geometry":null,"properties":{},"id":true})",
        R"({"type":"Feature","geometry":null,"properties":{},"id":null})",
        R"({"type":"FeatureCollection","features":[{"type":"Point","coordinates":[0,0]}]})",
        R"({"type":"GeometryCollection","geometries":[null]})"}) {
        EXPECT_THROW(read_geojson(text), std::invalid_argument);
    }
    for (const auto* text : {
        R"({"type":"Point","coordinates":[1,2]} trailing)",
        R"({"type":"Point","coordinates":[1,2)",
        R"({"type":"Point","type":"Point","coordinates":[1,2]})",
        R"({"type":"Point","coordinates":[1e999,2]})",
        R"({"type":"Point","coordinates":[NaN,2]})"}) {
        EXPECT_THROW(read_geojson(text), std::runtime_error);
    }
}

TEST(GisGeoJson, InvalidProgrammaticValuesAreRejected) {
    Feature feature;
    feature.properties = Value(1);
    EXPECT_THROW(write_geojson(feature), std::invalid_argument);
    feature.properties = Object{};
    feature.id = Value(std::numeric_limits<double>::infinity());
    EXPECT_THROW(write_geojson(feature), std::invalid_argument);
    feature.id.reset();
    feature.foreign_members.set("type", "Point");
    EXPECT_THROW(write_geojson(feature), std::invalid_argument);
    feature.foreign_members.remove("type");
    feature.foreign_members.set("id", "override");
    EXPECT_THROW(write_geojson(feature), std::invalid_argument);
    EXPECT_THROW(write_geojson(Geometry{Coordinate{0, 100, {}}, {}}), std::invalid_argument);
    EXPECT_THROW(write_geojson(Geometry{LineString{}, {}}), std::invalid_argument);
}

TEST(GisGeometry, ExcessiveNestingIsRejected) {
    Geometry nested{Coordinate{}, {}};
    for (int i = 0; i < 65; ++i) {
        nested = Geometry{GeometryCollection{{nested}}, {}};
    }
    EXPECT_THROW(validate(nested), std::invalid_argument);
    EXPECT_THROW(bounds(nested), std::invalid_argument);
    EXPECT_THROW(write_geojson(nested), std::invalid_argument);
    std::string text = R"({"type":"Point","coordinates":[0,0]})";
    for (int i = 0; i < 65; ++i) {
        text = R"({"type":"GeometryCollection","geometries":[)" + text + "]}";
    }
    EXPECT_THROW(read_geojson(text), std::runtime_error);
}
