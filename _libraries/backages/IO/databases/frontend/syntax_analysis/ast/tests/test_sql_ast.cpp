/**
 * @file test_sql_ast.cpp
 * @brief Unit tests for the SQL AST generator.
 *
 * Compile standalone:
 *   c++ -std=c++17 -I../headers -I../../parser/headers \
 *       ../../parser/source/sql_tokenizer.cpp ../source/sql_ast.cpp \
 *       test_sql_ast.cpp -o test_sql_ast
 */

#include "sql_ast.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace ml::sql::ast;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
#define PASS(name) std::cout << "PASS [" << (name) << "]\n"

static SelectStmt parse_select(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::SELECT);
    return std::get<SelectStmt>(stmts[0].node);
}

static InsertStmt parse_insert(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::INSERT);
    return std::get<InsertStmt>(stmts[0].node);
}

static UpdateStmt parse_update(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::UPDATE);
    return std::get<UpdateStmt>(stmts[0].node);
}

static DeleteStmt parse_delete(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::DELETE_STMT);
    return std::get<DeleteStmt>(stmts[0].node);
}

static CreateTableStmt parse_create_table(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::CREATE_TABLE);
    return std::get<CreateTableStmt>(stmts[0].node);
}

static DropTableStmt parse_drop_table(const std::string& sql) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(sql);
    assert(!stmts.empty());
    assert(stmts[0].type == StmtType::DROP_TABLE);
    return std::get<DropTableStmt>(stmts[0].node);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_simple_select() {
    auto s = parse_select("SELECT * FROM users;");
    assert(s.columns.size() == 1);
    assert(s.columns[0]->type == ExprType::STAR);
    assert(s.from_table == "users");
    assert(!s.where_clause);
    PASS("simple_select");
}

static void test_select_columns() {
    auto s = parse_select("SELECT id, name, email FROM users;");
    assert(s.columns.size() == 3);
    assert(s.columns[0]->value == "id");
    assert(s.columns[1]->value == "name");
    assert(s.columns[2]->value == "email");
    PASS("select_columns");
}

static void test_select_distinct() {
    auto s = parse_select("SELECT DISTINCT city FROM addresses;");
    assert(s.distinct);
    assert(s.columns.size() == 1);
    assert(s.columns[0]->value == "city");
    PASS("select_distinct");
}

static void test_select_where() {
    auto s = parse_select("SELECT * FROM users WHERE age >= 18;");
    assert(s.where_clause);
    assert(s.where_clause->type == ExprType::BINARY_OP);
    assert(s.where_clause->op == ">=");
    assert(s.where_clause->left->value == "age");
    assert(s.where_clause->right->value == "18");
    PASS("select_where");
}

static void test_select_where_and() {
    auto s = parse_select("SELECT * FROM users WHERE age >= 18 AND active = 1;");
    assert(s.where_clause->type == ExprType::BINARY_OP);
    assert(s.where_clause->op == "AND");
    assert(s.where_clause->left->op == ">=");
    assert(s.where_clause->right->op == "=");
    PASS("select_where_and");
}

static void test_select_order_by() {
    auto s = parse_select("SELECT * FROM users ORDER BY name ASC, age DESC;");
    assert(s.order_by.size() == 2);
    assert(s.order_by[0].ascending == true);
    assert(s.order_by[1].ascending == false);
    PASS("select_order_by");
}

static void test_select_limit_offset() {
    auto s = parse_select("SELECT * FROM users LIMIT 10 OFFSET 20;");
    assert(s.limit.has_value());
    assert(s.limit.value() == 10);
    assert(s.offset.has_value());
    assert(s.offset.value() == 20);
    PASS("select_limit_offset");
}

static void test_select_group_by_having() {
    auto s = parse_select("SELECT city, COUNT(*) FROM users GROUP BY city HAVING COUNT(*) > 5;");
    assert(s.group_by.size() == 1);
    assert(s.group_by[0]->value == "city");
    assert(s.having);
    assert(s.having->type == ExprType::BINARY_OP);
    assert(s.having->op == ">");
    PASS("select_group_by_having");
}

static void test_select_join() {
    auto s = parse_select(
        "SELECT u.name, o.total "
        "FROM users u "
        "INNER JOIN orders o ON u.id = o.user_id;");
    assert(s.from_table == "users");
    assert(s.from_alias == "u");
    assert(s.joins.size() == 1);
    assert(s.joins[0].type == JoinType::INNER);
    assert(s.joins[0].table == "orders");
    assert(s.joins[0].alias == "o");
    assert(s.joins[0].on_condition);
    PASS("select_join");
}

static void test_select_left_join() {
    auto s = parse_select(
        "SELECT * FROM a LEFT JOIN b ON a.id = b.a_id;");
    assert(s.joins.size() == 1);
    assert(s.joins[0].type == JoinType::LEFT);
    PASS("select_left_join");
}

static void test_select_alias() {
    auto s = parse_select("SELECT name AS user_name FROM users;");
    assert(s.columns.size() == 1);
    assert(s.columns[0]->alias == "user_name");
    PASS("select_alias");
}

static void test_select_function() {
    auto s = parse_select("SELECT COUNT(*), MAX(age) FROM users;");
    assert(s.columns.size() == 2);
    assert(s.columns[0]->type == ExprType::FUNCTION_CALL);
    assert(s.columns[0]->value == "COUNT");
    assert(s.columns[1]->type == ExprType::FUNCTION_CALL);
    assert(s.columns[1]->value == "MAX");
    PASS("select_function");
}

static void test_select_qualified_name() {
    auto s = parse_select("SELECT users.name FROM users;");
    assert(s.columns[0]->type == ExprType::IDENTIFIER);
    assert(s.columns[0]->value == "users.name");
    PASS("select_qualified_name");
}

static void test_select_like() {
    auto s = parse_select("SELECT * FROM users WHERE name LIKE '%john%';");
    assert(s.where_clause->type == ExprType::BINARY_OP);
    assert(s.where_clause->op == "LIKE");
    PASS("select_like");
}

static void test_select_between() {
    auto s = parse_select("SELECT * FROM users WHERE age BETWEEN 18 AND 65;");
    assert(s.where_clause->type == ExprType::BETWEEN);
    PASS("select_between");
}

static void test_select_in() {
    auto s = parse_select("SELECT * FROM users WHERE id IN (1, 2, 3);");
    assert(s.where_clause->type == ExprType::IN_LIST);
    assert(s.where_clause->args.size() == 3);
    PASS("select_in");
}

static void test_select_is_null() {
    auto s = parse_select("SELECT * FROM users WHERE email IS NULL;");
    assert(s.where_clause->type == ExprType::IS_NULL);
    assert(!s.where_clause->negated);
    PASS("select_is_null");
}

static void test_select_is_not_null() {
    auto s = parse_select("SELECT * FROM users WHERE email IS NOT NULL;");
    assert(s.where_clause->type == ExprType::IS_NULL);
    assert(s.where_clause->negated);
    PASS("select_is_not_null");
}

static void test_select_placeholder() {
    auto s = parse_select("SELECT * FROM users WHERE id = ?;");
    assert(s.where_clause->right->type == ExprType::PLACEHOLDER);
    assert(s.where_clause->right->value == "?");
    PASS("select_placeholder");
}

static void test_insert_simple() {
    auto s = parse_insert("INSERT INTO users (name, email) VALUES ('Alice', 'a@b.com');");
    assert(s.table == "users");
    assert(s.columns.size() == 2);
    assert(s.columns[0] == "name");
    assert(s.columns[1] == "email");
    assert(s.value_rows.size() == 1);
    assert(s.value_rows[0].size() == 2);
    assert(s.value_rows[0][0]->value == "Alice");
    PASS("insert_simple");
}

static void test_insert_multi_row() {
    auto s = parse_insert(
        "INSERT INTO users (name) VALUES ('Alice'), ('Bob'), ('Charlie');");
    assert(s.value_rows.size() == 3);
    assert(s.value_rows[0][0]->value == "Alice");
    assert(s.value_rows[1][0]->value == "Bob");
    assert(s.value_rows[2][0]->value == "Charlie");
    PASS("insert_multi_row");
}

static void test_update_simple() {
    auto s = parse_update("UPDATE users SET name = 'Bob' WHERE id = 1;");
    assert(s.table == "users");
    assert(s.assignments.size() == 1);
    assert(s.assignments[0].first == "name");
    assert(s.assignments[0].second->value == "Bob");
    assert(s.where_clause);
    PASS("update_simple");
}

static void test_update_multi() {
    auto s = parse_update("UPDATE users SET name = 'Bob', age = 30 WHERE id = 1;");
    assert(s.assignments.size() == 2);
    assert(s.assignments[1].first == "age");
    PASS("update_multi");
}

static void test_delete_simple() {
    auto s = parse_delete("DELETE FROM users WHERE id = 1;");
    assert(s.table == "users");
    assert(s.where_clause);
    assert(s.where_clause->op == "=");
    PASS("delete_simple");
}

static void test_delete_all() {
    auto s = parse_delete("DELETE FROM users;");
    assert(s.table == "users");
    assert(!s.where_clause);
    PASS("delete_all");
}

static void test_create_table() {
    auto s = parse_create_table(
        "CREATE TABLE products ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  price REAL DEFAULT 0.0,"
        "  sku TEXT UNIQUE"
        ");");
    assert(s.table == "products");
    assert(!s.if_not_exists);
    assert(s.columns.size() == 4);

    assert(s.columns[0].name == "id");
    assert(s.columns[0].type_name == "INTEGER");
    assert(s.columns[0].primary_key);
    assert(s.columns[0].auto_increment);

    assert(s.columns[1].name == "name");
    assert(s.columns[1].type_name == "TEXT");
    assert(s.columns[1].not_null);

    assert(s.columns[2].name == "price");
    assert(s.columns[2].type_name == "REAL");
    assert(s.columns[2].default_value == "0.0");

    assert(s.columns[3].name == "sku");
    assert(s.columns[3].unique);

    PASS("create_table");
}

static void test_create_table_if_not_exists() {
    auto s = parse_create_table(
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY);");
    assert(s.if_not_exists);
    assert(s.table == "users");
    PASS("create_table_if_not_exists");
}

static void test_drop_table() {
    auto s = parse_drop_table("DROP TABLE users;");
    assert(s.table == "users");
    assert(!s.if_exists);
    PASS("drop_table");
}

static void test_drop_table_if_exists() {
    auto s = parse_drop_table("DROP TABLE IF EXISTS users;");
    assert(s.table == "users");
    assert(s.if_exists);
    PASS("drop_table_if_exists");
}

static void test_multiple_statements() {
    SQLAstGenerator gen;
    auto stmts = gen.parse("SELECT 1; SELECT 2; SELECT 3;");
    assert(stmts.size() == 3);
    for (auto& s : stmts) {
        assert(s.type == StmtType::SELECT);
    }
    PASS("multiple_statements");
}

static void test_arithmetic_expr() {
    auto s = parse_select("SELECT price * 1.1 FROM products;");
    assert(s.columns[0]->type == ExprType::BINARY_OP);
    assert(s.columns[0]->op == "*");
    PASS("arithmetic_expr");
}

static void test_nested_parens() {
    auto s = parse_select("SELECT * FROM t WHERE (a = 1 OR b = 2) AND c = 3;");
    assert(s.where_clause->op == "AND");
    assert(s.where_clause->left->op == "OR");
    PASS("nested_parens");
}

static void test_not_expr() {
    auto s = parse_select("SELECT * FROM t WHERE NOT active;");
    assert(s.where_clause->type == ExprType::UNARY_OP);
    assert(s.where_clause->op == "NOT");
    PASS("not_expr");
}

static void test_negative_number() {
    auto s = parse_select("SELECT * FROM t WHERE val = -5;");
    assert(s.where_clause->right->type == ExprType::UNARY_OP);
    assert(s.where_clause->right->op == "-");
    PASS("negative_number");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== SQL AST Generator Tests ===\n\n";

    test_simple_select();
    test_select_columns();
    test_select_distinct();
    test_select_where();
    test_select_where_and();
    test_select_order_by();
    test_select_limit_offset();
    test_select_group_by_having();
    test_select_join();
    test_select_left_join();
    test_select_alias();
    test_select_function();
    test_select_qualified_name();
    test_select_like();
    test_select_between();
    test_select_in();
    test_select_is_null();
    test_select_is_not_null();
    test_select_placeholder();
    test_insert_simple();
    test_insert_multi_row();
    test_update_simple();
    test_update_multi();
    test_delete_simple();
    test_delete_all();
    test_create_table();
    test_create_table_if_not_exists();
    test_drop_table();
    test_drop_table_if_exists();
    test_multiple_statements();
    test_arithmetic_expr();
    test_nested_parens();
    test_not_expr();
    test_negative_number();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
