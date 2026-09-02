/**
 * @file test_grammar_validation.cpp
 * @brief Unit tests for the SQL grammar validator.
 *
 * Compile standalone:
 *   c++ -std=c++17 -I../headers -I../../../syntax_analysis/ast/headers \
 *       -I../../../syntax_analysis/parser/headers \
 *       ../../../syntax_analysis/parser/source/sql_tokenizer.cpp \
 *       ../../../syntax_analysis/ast/source/sql_ast.cpp \
 *       ../source/grammar_validation.cpp \
 *       test_grammar_validation.cpp -o test_grammar_validation
 */

#include "grammar_validation.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace ml::sql::validation;

#define PASS(name) std::cout << "PASS [" << (name) << "]\n"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static ValidationResult validate(const std::string& sql) {
    GrammarValidator v;
    return v.validate_sql(sql);
}

static bool has_rule(const ValidationResult& r, const std::string& rule_id) {
    for (auto& d : r.diagnostics) {
        if (d.rule == rule_id) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_valid_select() {
    auto r = validate("SELECT * FROM users;");
    assert(r.valid);
    assert(r.error_count() == 0);
    PASS("valid_select");
}

static void test_valid_insert() {
    auto r = validate("INSERT INTO users (name) VALUES ('Alice');");
    assert(r.valid);
    assert(r.error_count() == 0);
    PASS("valid_insert");
}

static void test_valid_update_with_where() {
    auto r = validate("UPDATE users SET name = 'Bob' WHERE id = 1;");
    assert(r.valid);
    assert(r.warning_count() == 0);
    PASS("valid_update_with_where");
}

static void test_valid_delete_with_where() {
    auto r = validate("DELETE FROM users WHERE id = 1;");
    assert(r.valid);
    assert(r.warning_count() == 0);
    PASS("valid_delete_with_where");
}

static void test_valid_create_table() {
    auto r = validate("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);");
    assert(r.valid);
    assert(r.error_count() == 0);
    PASS("valid_create_table");
}

static void test_valid_drop_table() {
    auto r = validate("DROP TABLE users;");
    assert(r.valid);
    PASS("valid_drop_table");
}

static void test_update_no_where_warning() {
    auto r = validate("UPDATE users SET name = 'Bob';");
    assert(r.valid); // warnings don't make it invalid
    assert(has_rule(r, "UPDATE_NO_WHERE"));
    PASS("update_no_where_warning");
}

static void test_delete_no_where_warning() {
    auto r = validate("DELETE FROM users;");
    assert(r.valid); // warnings don't make it invalid
    assert(has_rule(r, "DELETE_NO_WHERE"));
    PASS("delete_no_where_warning");
}

static void test_insert_column_value_mismatch() {
    auto r = validate("INSERT INTO users (name, email) VALUES ('Alice');");
    assert(!r.valid);
    assert(has_rule(r, "INSERT_COLUMN_VALUE_MISMATCH"));
    PASS("insert_column_value_mismatch");
}

static void test_select_star_group_by_warning() {
    auto r = validate("SELECT * FROM users GROUP BY city;");
    assert(r.valid); // it's a warning
    assert(has_rule(r, "SELECT_STAR_WITH_GROUP_BY"));
    PASS("select_star_group_by_warning");
}

static void test_join_no_on_warning() {
    auto r = validate("SELECT * FROM a JOIN b;");
    assert(r.valid); // warning only
    assert(has_rule(r, "JOIN_NO_ON"));
    PASS("join_no_on_warning");
}

static void test_join_with_on_no_warning() {
    auto r = validate("SELECT * FROM a INNER JOIN b ON a.id = b.a_id;");
    assert(r.valid);
    assert(!has_rule(r, "JOIN_NO_ON"));
    PASS("join_with_on_no_warning");
}

static void test_cross_join_no_warning() {
    auto r = validate("SELECT * FROM a CROSS JOIN b;");
    assert(r.valid);
    assert(!has_rule(r, "JOIN_NO_ON"));
    PASS("cross_join_no_warning");
}

static void test_ambiguous_column_info() {
    auto r = validate("SELECT name FROM users INNER JOIN orders ON users.id = orders.user_id;");
    assert(r.valid);
    assert(has_rule(r, "AMBIGUOUS_COLUMN"));
    PASS("ambiguous_column_info");
}

static void test_qualified_column_no_ambiguity() {
    auto r = validate("SELECT users.name FROM users INNER JOIN orders ON users.id = orders.user_id;");
    assert(r.valid);
    assert(!has_rule(r, "AMBIGUOUS_COLUMN"));
    PASS("qualified_column_no_ambiguity");
}

static void test_errors_only_mode() {
    GrammarValidator v;
    v.errors_only();
    auto r = v.validate_sql("DELETE FROM users;");
    // DELETE_NO_WHERE is a warning, should be suppressed
    assert(!has_rule(r, "DELETE_NO_WHERE"));
    PASS("errors_only_mode");
}

static void test_disable_rule() {
    GrammarValidator v;
    v.set_rule_enabled("UPDATE_NO_WHERE", false);
    auto r = v.validate_sql("UPDATE users SET name = 'x';");
    assert(!has_rule(r, "UPDATE_NO_WHERE"));
    PASS("disable_rule");
}

static void test_custom_rule() {
    GrammarValidator v;

    Rule custom;
    custom.id = "NO_TRUNCATE";
    custom.description = "TRUNCATE not allowed";
    custom.default_severity = Severity::ERROR;
    custom.check = [](const Statement& /*s*/, ValidationResult& /*r*/) {
        // Would check for truncate — just a placeholder
    };
    custom.enabled = true;
    v.add_rule(std::move(custom));

    assert(v.rules().size() > 0);
    bool found = false;
    for (auto& rule : v.rules()) {
        if (rule.id == "NO_TRUNCATE") { found = true; break; }
    }
    assert(found);
    PASS("custom_rule");
}

static void test_parse_error() {
    auto r = validate("SELEC * FORM users;"); // typo
    assert(!r.valid);
    assert(has_rule(r, "PARSE_ERROR"));
    PASS("parse_error");
}

static void test_multiple_statements() {
    auto r = validate("SELECT * FROM a; DELETE FROM b;");
    // SELECT is valid, DELETE has no WHERE warning
    assert(r.valid);
    assert(has_rule(r, "DELETE_NO_WHERE"));
    PASS("multiple_statements");
}

static void test_select_literal_no_from_ok() {
    auto r = validate("SELECT 1;");
    // SELECT literal without FROM should NOT trigger SELECT_NO_FROM
    assert(r.valid);
    assert(!has_rule(r, "SELECT_NO_FROM"));
    PASS("select_literal_no_from_ok");
}

static void test_severity_counts() {
    auto r = validate("SELECT * FROM a JOIN b; DELETE FROM c;");
    // JOIN_NO_ON = warning, DELETE_NO_WHERE = warning
    assert(r.valid);
    assert(r.warning_count() >= 2);
    assert(r.error_count() == 0);
    PASS("severity_counts");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== Grammar Validation Tests ===\n\n";

    test_valid_select();
    test_valid_insert();
    test_valid_update_with_where();
    test_valid_delete_with_where();
    test_valid_create_table();
    test_valid_drop_table();
    test_update_no_where_warning();
    test_delete_no_where_warning();
    test_insert_column_value_mismatch();
    test_select_star_group_by_warning();
    test_join_no_on_warning();
    test_join_with_on_no_warning();
    test_cross_join_no_warning();
    test_ambiguous_column_info();
    test_qualified_column_no_ambiguity();
    test_errors_only_mode();
    test_disable_rule();
    test_custom_rule();
    test_parse_error();
    test_multiple_statements();
    test_select_literal_no_from_ok();
    test_severity_counts();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
