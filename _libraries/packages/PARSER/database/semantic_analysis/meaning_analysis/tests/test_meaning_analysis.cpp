/**
 * @file test_meaning_analysis.cpp
 * @brief Unit tests for the SQL meaning / semantic analyzer.
 *
 * Compile standalone:
 *   c++ -std=c++17 -I../headers -I../../../syntax_analysis/ast/headers \
 *       -I../../../lexical_analysis/parser/headers \
 *       ../../../lexical_analysis/parser/source/sql_tokenizer.cpp \
 *       ../../../syntax_analysis/ast/source/sql_ast.cpp \
 *       ../source/meaning_analysis.cpp \
 *       test_meaning_analysis.cpp -o test_meaning_analysis
 */

#include "meaning_analysis.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace ml::sql::semantic;
using namespace ml::sql::ast;

#define PASS(name) std::cout << "PASS [" << (name) << "]\n"

// ---------------------------------------------------------------------------
// Shared schema: users(id INTEGER PK, name TEXT NOT NULL, email TEXT, age INTEGER)
//                orders(id INTEGER PK, user_id INTEGER NOT NULL, total REAL)
// ---------------------------------------------------------------------------
static SchemaCatalog make_test_catalog() {
    SchemaCatalog cat;

    CatalogTable users;
    users.table_name = "users";
    users.columns = {
        {"id",    DataType::INTEGER, false, true,  true},
        {"name",  DataType::TEXT,    false, false, false},
        {"email", DataType::TEXT,    true,  false, false},
        {"age",   DataType::INTEGER, true,  false, false},
    };
    cat.add_table(users);

    CatalogTable orders;
    orders.table_name = "orders";
    orders.columns = {
        {"id",      DataType::INTEGER, false, true,  true},
        {"user_id", DataType::INTEGER, false, false, false},
        {"total",   DataType::REAL,    true,  false, false},
    };
    cat.add_table(orders);

    return cat;
}

static bool has_rule(const SemanticResult& r, const std::string& rule_id) {
    for (auto& d : r.diagnostics) {
        if (d.rule == rule_id) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tests: SchemaCatalog
// ---------------------------------------------------------------------------

static void test_catalog_from_sql() {
    auto cat = SchemaCatalog::from_sql(
        "CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT NOT NULL, price REAL);");
    assert(cat.has_table("products"));
    auto* t = cat.find_table("products");
    assert(t != nullptr);
    assert(t->columns.size() == 3);
    assert(t->has_column("id"));
    assert(t->has_column("name"));
    assert(t->has_column("price"));
    assert(!t->has_column("nonexistent"));
    PASS("catalog_from_sql");
}

static void test_catalog_case_insensitive() {
    auto cat = make_test_catalog();
    assert(cat.has_table("USERS"));
    assert(cat.has_table("Users"));
    auto* t = cat.find_table("users");
    assert(t->has_column("ID"));
    assert(t->has_column("Name"));
    PASS("catalog_case_insensitive");
}

static void test_catalog_table_names() {
    auto cat = make_test_catalog();
    auto names = cat.table_names();
    assert(names.size() == 2);
    PASS("catalog_table_names");
}

static void test_catalog_remove_table() {
    auto cat = make_test_catalog();
    assert(cat.has_table("users"));
    cat.remove_table("users");
    assert(!cat.has_table("users"));
    PASS("catalog_remove_table");
}

// ---------------------------------------------------------------------------
// Tests: TABLE_NOT_FOUND
// ---------------------------------------------------------------------------

static void test_select_valid_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM users;");
    assert(r.valid);
    assert(!has_rule(r, "TABLE_NOT_FOUND"));
    PASS("select_valid_table");
}

static void test_select_invalid_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM nonexistent;");
    assert(!r.valid);
    assert(has_rule(r, "TABLE_NOT_FOUND"));
    PASS("select_invalid_table");
}

static void test_join_invalid_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM users INNER JOIN ghost ON users.id = ghost.uid;");
    assert(!r.valid);
    assert(has_rule(r, "TABLE_NOT_FOUND"));
    PASS("join_invalid_table");
}

static void test_insert_invalid_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("INSERT INTO ghost (name) VALUES ('x');");
    assert(!r.valid);
    assert(has_rule(r, "TABLE_NOT_FOUND"));
    PASS("insert_invalid_table");
}

// ---------------------------------------------------------------------------
// Tests: COLUMN_NOT_FOUND
// ---------------------------------------------------------------------------

static void test_select_valid_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT name, email FROM users;");
    assert(!has_rule(r, "COLUMN_NOT_FOUND"));
    PASS("select_valid_column");
}

static void test_select_invalid_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT nonexistent FROM users;");
    assert(!r.valid);
    assert(has_rule(r, "COLUMN_NOT_FOUND"));
    PASS("select_invalid_column");
}

static void test_insert_invalid_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("INSERT INTO users (name, ghost_col) VALUES ('x', 'y');");
    assert(!r.valid);
    assert(has_rule(r, "COLUMN_NOT_FOUND"));
    PASS("insert_invalid_column");
}

static void test_update_invalid_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("UPDATE users SET ghost_col = 'x' WHERE id = 1;");
    assert(!r.valid);
    assert(has_rule(r, "COLUMN_NOT_FOUND"));
    PASS("update_invalid_column");
}

// ---------------------------------------------------------------------------
// Tests: DUPLICATE_INSERT_COLUMN
// ---------------------------------------------------------------------------

static void test_duplicate_insert_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("INSERT INTO users (name, name) VALUES ('a', 'b');");
    assert(!r.valid);
    assert(has_rule(r, "DUPLICATE_INSERT_COLUMN"));
    PASS("duplicate_insert_column");
}

// ---------------------------------------------------------------------------
// Tests: MISSING_NOT_NULL_COLUMN
// ---------------------------------------------------------------------------

static void test_missing_not_null_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    // 'name' is NOT NULL but not provided
    auto r = ma.analyze_sql("INSERT INTO users (email) VALUES ('test@test.com');");
    assert(r.valid); // it's a warning
    assert(has_rule(r, "MISSING_NOT_NULL_COLUMN"));
    PASS("missing_not_null_column");
}

static void test_all_not_null_columns_present() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("INSERT INTO users (name, email) VALUES ('Alice', 'a@b.com');");
    assert(!has_rule(r, "MISSING_NOT_NULL_COLUMN"));
    PASS("all_not_null_columns_present");
}

// ---------------------------------------------------------------------------
// Tests: AGGREGATE_IN_WHERE
// ---------------------------------------------------------------------------

static void test_aggregate_in_where() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM users WHERE COUNT(*) > 5;");
    assert(!r.valid);
    assert(has_rule(r, "AGGREGATE_IN_WHERE"));
    PASS("aggregate_in_where");
}

static void test_no_aggregate_in_where() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM users WHERE age > 18;");
    assert(!has_rule(r, "AGGREGATE_IN_WHERE"));
    PASS("no_aggregate_in_where");
}

// ---------------------------------------------------------------------------
// Tests: NON_AGGREGATED_COLUMN
// ---------------------------------------------------------------------------

static void test_non_aggregated_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    // 'email' is not in GROUP BY and not aggregated
    auto r = ma.analyze_sql("SELECT name, email, COUNT(*) FROM users GROUP BY name;");
    assert(has_rule(r, "NON_AGGREGATED_COLUMN"));
    PASS("non_aggregated_column");
}

static void test_properly_grouped_column() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT name, COUNT(*) FROM users GROUP BY name;");
    assert(!has_rule(r, "NON_AGGREGATED_COLUMN"));
    PASS("properly_grouped_column");
}

// ---------------------------------------------------------------------------
// Tests: TYPE_MISMATCH_COMPARISON
// ---------------------------------------------------------------------------

static void test_type_mismatch() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    // age is INTEGER, comparing with TEXT literal
    auto r = ma.analyze_sql("SELECT * FROM users WHERE age = 'hello';");
    assert(has_rule(r, "TYPE_MISMATCH_COMPARISON"));
    PASS("type_mismatch");
}

static void test_type_match() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("SELECT * FROM users WHERE age = 25;");
    assert(!has_rule(r, "TYPE_MISMATCH_COMPARISON"));
    PASS("type_match");
}

static void test_numeric_cross_compat() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    // total is REAL, comparing with INTEGER — should be fine
    auto r = ma.analyze_sql("SELECT * FROM orders WHERE total > 100;");
    assert(!has_rule(r, "TYPE_MISMATCH_COMPARISON"));
    PASS("numeric_cross_compat");
}

// ---------------------------------------------------------------------------
// Tests: MODIFY_NONEXISTENT_TABLE
// ---------------------------------------------------------------------------

static void test_update_nonexistent_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("UPDATE ghost SET name = 'x';");
    assert(!r.valid);
    assert(has_rule(r, "MODIFY_NONEXISTENT_TABLE"));
    PASS("update_nonexistent_table");
}

static void test_delete_nonexistent_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("DELETE FROM ghost WHERE id = 1;");
    assert(!r.valid);
    assert(has_rule(r, "MODIFY_NONEXISTENT_TABLE"));
    PASS("delete_nonexistent_table");
}

// ---------------------------------------------------------------------------
// Tests: DROP_NONEXISTENT_TABLE
// ---------------------------------------------------------------------------

static void test_drop_nonexistent_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("DROP TABLE ghost;");
    assert(!r.valid);
    assert(has_rule(r, "DROP_NONEXISTENT_TABLE"));
    PASS("drop_nonexistent_table");
}

static void test_drop_if_exists_no_error() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("DROP TABLE IF EXISTS ghost;");
    assert(!has_rule(r, "DROP_NONEXISTENT_TABLE"));
    PASS("drop_if_exists_no_error");
}

// ---------------------------------------------------------------------------
// Tests: CREATE_EXISTING_TABLE
// ---------------------------------------------------------------------------

static void test_create_existing_table() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("CREATE TABLE users (id INTEGER PRIMARY KEY);");
    assert(!r.valid);
    assert(has_rule(r, "CREATE_EXISTING_TABLE"));
    PASS("create_existing_table");
}

static void test_create_if_not_exists_no_error() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY);");
    assert(!has_rule(r, "CREATE_EXISTING_TABLE"));
    PASS("create_if_not_exists_no_error");
}

// ---------------------------------------------------------------------------
// Tests: DUPLICATE_COLUMN_DEF
// ---------------------------------------------------------------------------

static void test_duplicate_column_def() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    auto r = ma.analyze_sql("CREATE TABLE IF NOT EXISTS t (id INTEGER, name TEXT, id INTEGER);");
    assert(!r.valid);
    assert(has_rule(r, "DUPLICATE_COLUMN_DEF"));
    PASS("duplicate_column_def");
}

// ---------------------------------------------------------------------------
// Tests: Custom rule and disable
// ---------------------------------------------------------------------------

static void test_custom_rule() {
    MeaningAnalyzer ma;
    SemanticRule custom;
    custom.id = "CUSTOM_CHECK";
    custom.description = "Test custom rule";
    custom.default_severity = SemanticSeverity::INFO;
    custom.check = [](const Statement&, const SchemaCatalog&, SemanticResult& r) {
        SemanticDiagnostic d;
        d.severity = SemanticSeverity::INFO;
        d.rule = "CUSTOM_CHECK";
        d.message = "Custom rule fired";
        r.diagnostics.push_back(d);
    };
    custom.enabled = true;
    ma.add_rule(std::move(custom));

    auto r = ma.analyze_sql("SELECT 1;");
    assert(has_rule(r, "CUSTOM_CHECK"));
    PASS("custom_rule");
}

static void test_disable_rule() {
    auto cat = make_test_catalog();
    MeaningAnalyzer ma;
    ma.set_catalog(cat);
    ma.set_rule_enabled("TABLE_NOT_FOUND", false);

    auto r = ma.analyze_sql("SELECT * FROM ghost;");
    assert(!has_rule(r, "TABLE_NOT_FOUND"));
    PASS("disable_rule");
}

// ---------------------------------------------------------------------------
// Tests: Empty catalog (no schema — rules should be lenient)
// ---------------------------------------------------------------------------

static void test_no_catalog_select() {
    MeaningAnalyzer ma; // no catalog set
    auto r = ma.analyze_sql("SELECT * FROM anything;");
    // TABLE_NOT_FOUND fires because catalog is empty
    assert(has_rule(r, "TABLE_NOT_FOUND"));
    PASS("no_catalog_select");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== Meaning Analysis Tests ===\n\n";

    // Catalog tests
    test_catalog_from_sql();
    test_catalog_case_insensitive();
    test_catalog_table_names();
    test_catalog_remove_table();

    // Table existence
    test_select_valid_table();
    test_select_invalid_table();
    test_join_invalid_table();
    test_insert_invalid_table();

    // Column existence
    test_select_valid_column();
    test_select_invalid_column();
    test_insert_invalid_column();
    test_update_invalid_column();

    // Duplicate insert column
    test_duplicate_insert_column();

    // Missing NOT NULL
    test_missing_not_null_column();
    test_all_not_null_columns_present();

    // Aggregate in WHERE
    test_aggregate_in_where();
    test_no_aggregate_in_where();

    // Non-aggregated column
    test_non_aggregated_column();
    test_properly_grouped_column();

    // Type mismatch
    test_type_mismatch();
    test_type_match();
    test_numeric_cross_compat();

    // Modify nonexistent
    test_update_nonexistent_table();
    test_delete_nonexistent_table();

    // Drop nonexistent
    test_drop_nonexistent_table();
    test_drop_if_exists_no_error();

    // Create existing
    test_create_existing_table();
    test_create_if_not_exists_no_error();

    // Duplicate column def
    test_duplicate_column_def();

    // Custom / disable
    test_custom_rule();
    test_disable_rule();

    // No catalog
    test_no_catalog_select();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
