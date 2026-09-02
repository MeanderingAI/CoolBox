#include "../headers/bayesian_network.h"
#include "tyst_framework.hpp"

using namespace bayesian_db;

TEST(BayesianNetworkDBTest, TableInsertAndQuery) {
    BayesianNetworkDB db;
    db.add_table("people", {"name", "age", "city"});
    db.insert("people", {{"name", "Alice"}, {"age", "30"}, {"city", "NY"}});
    db.insert("people", {{"name", "Bob"}, {"age", "25"}, {"city", "LA"}});
    auto results = db.query("people", "city", "NY");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0]["name"], "Alice");
}

TEST(BayesianNetworkDBTest, TableToMatrixAndDecode) {
    BayesianNetworkDB db;
    db.add_table("people", {"name", "age", "city"});
    db.insert("people", {{"name", "Alice"}, {"age", "30"}, {"city", "NY"}});
    db.insert("people", {{"name", "Bob"}, {"age", "25"}, {"city", "LA"}});
    auto mat = db.table_to_matrix("people");
    ASSERT_EQ(mat->rows(), 2);
    ASSERT_EQ(mat->cols(), 3);
    // Decode first row
    std::vector<int> row0 = {static_cast<int>(mat->at(0,0)), static_cast<int>(mat->at(0,1)), static_cast<int>(mat->at(0,2))};
    auto decoded = db.decode_row("people", row0);
    EXPECT_EQ(decoded["name"], "Alice");
    EXPECT_EQ(decoded["city"], "NY");
}

TEST(BayesianNetworkDBTest, MAPandMLE) {
    BayesianNetworkDB db;
    db.add_table("people", {"name", "age", "city"});
    db.insert("people", {{"name", "Alice"}, {"age", "30"}, {"city", "NY"}});
    db.insert("people", {{"name", "Alice"}, {"age", "30"}, {"city", "NY"}});
    db.insert("people", {{"name", "Bob"}, {"age", "25"}, {"city", "LA"}});
    std::unordered_map<std::string, std::string> evidence = {{"name", "Alice"}};
    auto map_result = db.map("people", evidence);
    ASSERT_TRUE(map_result.has_value());
    EXPECT_EQ(map_result.value().at("city"), "NY");
    auto mle_result = db.mle("people", evidence);
    ASSERT_TRUE(mle_result.has_value());
    EXPECT_EQ(mle_result.value().at("city"), "NY");
}
