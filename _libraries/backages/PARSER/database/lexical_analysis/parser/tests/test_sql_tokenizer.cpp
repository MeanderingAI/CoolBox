/**
 * @file test_sql_tokenizer.cpp
 * @brief Unit tests for the SQL tokenizer.
 *
 * Compile standalone:
 *   c++ -std=c++17 -I../headers ../source/sql_tokenizer.cpp test_sql_tokenizer.cpp -o test_sql_tokenizer
 */

#include "sql_tokenizer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace ml::sql::parser;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void expect_types(const std::vector<Token>& tokens,
                         const std::vector<TokenType>& expected,
                         const char* test_name) {
    if (tokens.size() != expected.size()) {
        std::cerr << "FAIL [" << test_name << "] expected "
                  << expected.size() << " tokens, got " << tokens.size() << "\n";
        for (auto& t : tokens) std::cerr << "  " << t << "\n";
        assert(false);
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        if (tokens[i].type != expected[i]) {
            std::cerr << "FAIL [" << test_name << "] token " << i
                      << ": expected " << token_type_name(expected[i])
                      << ", got " << tokens[i] << "\n";
            assert(false);
        }
    }
    std::cout << "PASS [" << test_name << "]\n";
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_simple_select() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT * FROM users;");

    expect_types(tokens, {
        TokenType::KEYWORD,       // SELECT
        TokenType::STAR,          // *
        TokenType::KEYWORD,       // FROM
        TokenType::IDENTIFIER,    // users
        TokenType::SEMICOLON,     // ;
        TokenType::END_OF_INPUT,
    }, "simple_select");

    assert(tokens[0].value == "SELECT");
    assert(tokens[3].value == "users");
}

static void test_select_with_where() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT id, name FROM users WHERE age >= 18 AND active = 1;");

    expect_types(tokens, {
        TokenType::KEYWORD,         // SELECT
        TokenType::IDENTIFIER,      // id
        TokenType::COMMA,           // ,
        TokenType::IDENTIFIER,      // name
        TokenType::KEYWORD,         // FROM
        TokenType::IDENTIFIER,      // users
        TokenType::KEYWORD,         // WHERE
        TokenType::IDENTIFIER,      // age
        TokenType::GREATER_EQUAL,   // >=
        TokenType::NUMERIC_LITERAL, // 18
        TokenType::KEYWORD,         // AND
        TokenType::IDENTIFIER,      // active
        TokenType::EQUALS,          // =
        TokenType::NUMERIC_LITERAL, // 1
        TokenType::SEMICOLON,       // ;
        TokenType::END_OF_INPUT,
    }, "select_with_where");
}

static void test_insert() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("INSERT INTO users (name, email) VALUES ('Alice', 'a@b.com');");

    expect_types(tokens, {
        TokenType::KEYWORD,        // INSERT
        TokenType::KEYWORD,        // INTO
        TokenType::IDENTIFIER,     // users
        TokenType::OPEN_PAREN,     // (
        TokenType::IDENTIFIER,     // name
        TokenType::COMMA,          // ,
        TokenType::IDENTIFIER,     // email
        TokenType::CLOSE_PAREN,    // )
        TokenType::KEYWORD,        // VALUES
        TokenType::OPEN_PAREN,     // (
        TokenType::STRING_LITERAL, // Alice
        TokenType::COMMA,          // ,
        TokenType::STRING_LITERAL, // a@b.com
        TokenType::CLOSE_PAREN,    // )
        TokenType::SEMICOLON,      // ;
        TokenType::END_OF_INPUT,
    }, "insert");

    assert(tokens[10].value == "Alice");
    assert(tokens[12].value == "a@b.com");
}

static void test_string_escape() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT 'it''s a test';");

    assert(tokens[1].type == TokenType::STRING_LITERAL);
    assert(tokens[1].value == "it's a test");
    std::cout << "PASS [string_escape]\n";
}

static void test_numeric_literals() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT 42, 3.14, 0xFF, 1e10;");

    assert(tokens[1].type == TokenType::NUMERIC_LITERAL);
    assert(tokens[1].value == "42");
    assert(tokens[3].type == TokenType::NUMERIC_LITERAL);
    assert(tokens[3].value == "3.14");
    assert(tokens[5].type == TokenType::NUMERIC_LITERAL);
    assert(tokens[5].value == "0xFF");
    assert(tokens[7].type == TokenType::NUMERIC_LITERAL);
    assert(tokens[7].value == "1e10");
    std::cout << "PASS [numeric_literals]\n";
}

static void test_operators() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("a != b AND c <> d AND e <= f AND g >= h");

    // a != b AND c <> d AND e <= f AND g >= h
    assert(tokens[1].type == TokenType::NOT_EQUALS);
    assert(tokens[1].value == "!=");
    assert(tokens[5].type == TokenType::NOT_EQUALS);
    assert(tokens[5].value == "<>");
    assert(tokens[9].type == TokenType::LESS_EQUAL);
    assert(tokens[13].type == TokenType::GREATER_EQUAL);
    std::cout << "PASS [operators]\n";
}

static void test_comments_stripped() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT -- this is a comment\n* FROM t;");

    // Comments stripped by default
    expect_types(tokens, {
        TokenType::KEYWORD,      // SELECT
        TokenType::STAR,         // *
        TokenType::KEYWORD,      // FROM
        TokenType::IDENTIFIER,   // t
        TokenType::SEMICOLON,    // ;
        TokenType::END_OF_INPUT,
    }, "comments_stripped");
}

static void test_comments_preserved() {
    TokenizerConfig cfg;
    cfg.preserve_comments = true;
    SQLTokenizer tok(cfg);
    auto tokens = tok.tokenize("SELECT /* block */ 1;");

    bool found_comment = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::BLOCK_COMMENT) {
            found_comment = true;
            assert(t.value == "/* block */");
        }
    }
    assert(found_comment);
    std::cout << "PASS [comments_preserved]\n";
}

static void test_whitespace_preserved() {
    TokenizerConfig cfg;
    cfg.preserve_whitespace = true;
    SQLTokenizer tok(cfg);
    auto tokens = tok.tokenize("SELECT  *");

    bool found_ws = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::WHITESPACE) {
            found_ws = true;
            break;
        }
    }
    assert(found_ws);
    std::cout << "PASS [whitespace_preserved]\n";
}

static void test_placeholders() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT * FROM t WHERE id = ? AND name = :name AND x = $1;");

    int ph_count = 0;
    for (auto& t : tokens) {
        if (t.type == TokenType::PLACEHOLDER) ++ph_count;
    }
    assert(ph_count == 3);
    std::cout << "PASS [placeholders]\n";
}

static void test_quoted_identifiers() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT \"my col\" FROM `my_table`;");

    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].value == "my col");
    assert(tokens[3].type == TokenType::IDENTIFIER);
    assert(tokens[3].value == "my_table");
    std::cout << "PASS [quoted_identifiers]\n";
}

static void test_create_table() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize(
        "CREATE TABLE IF NOT EXISTS products (\n"
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "    name TEXT NOT NULL,\n"
        "    price REAL DEFAULT 0.0\n"
        ");");

    // Just verify it tokenizes without crashing and has correct structure
    assert(tokens.front().type == TokenType::KEYWORD);
    assert(tokens.front().value == "CREATE");
    assert(tokens.back().type == TokenType::END_OF_INPUT);

    // Find the semicolon
    bool found_semi = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::SEMICOLON) { found_semi = true; break; }
    }
    assert(found_semi);
    std::cout << "PASS [create_table]\n";
}

static void test_blob_literal() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT X'1A2B3C';");

    assert(tokens[1].type == TokenType::BLOB_LITERAL);
    assert(tokens[1].value == "X'1A2B3C'");
    std::cout << "PASS [blob_literal]\n";
}

static void test_line_column_tracking() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT\n  *\nFROM t;");

    // SELECT is at line 1
    assert(tokens[0].line == 1);
    // * is at line 2
    assert(tokens[1].line == 2);
    // FROM is at line 3
    assert(tokens[2].line == 3);
    std::cout << "PASS [line_column_tracking]\n";
}

static void test_tokenize_stripped() {
    TokenizerConfig cfg;
    cfg.preserve_whitespace = true;
    cfg.preserve_comments   = true;
    SQLTokenizer tok(cfg);

    auto tokens = tok.tokenize_stripped("SELECT  /* x */ * FROM t;");

    for (auto& t : tokens) {
        assert(t.type != TokenType::WHITESPACE);
        assert(t.type != TokenType::BLOCK_COMMENT);
    }
    std::cout << "PASS [tokenize_stripped]\n";
}

static void test_multiple_statements() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("SELECT 1; SELECT 2;");

    int semi_count = 0;
    for (auto& t : tokens) {
        if (t.type == TokenType::SEMICOLON) ++semi_count;
    }
    assert(semi_count == 2);
    std::cout << "PASS [multiple_statements]\n";
}

static void test_join_query() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize(
        "SELECT u.name, o.total "
        "FROM users u "
        "INNER JOIN orders o ON u.id = o.user_id "
        "WHERE o.total > 100;");

    // Verify DOT tokens for qualified names
    int dot_count = 0;
    for (auto& t : tokens) {
        if (t.type == TokenType::DOT) ++dot_count;
    }
    assert(dot_count == 5); // u.name, o.total, u.id, o.user_id, o.total
    std::cout << "PASS [join_query]\n";
}

static void test_empty_input() {
    SQLTokenizer tok;
    auto tokens = tok.tokenize("");

    assert(tokens.size() == 1);
    assert(tokens[0].type == TokenType::END_OF_INPUT);
    std::cout << "PASS [empty_input]\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== SQL Tokenizer Tests ===\n\n";

    test_simple_select();
    test_select_with_where();
    test_insert();
    test_string_escape();
    test_numeric_literals();
    test_operators();
    test_comments_stripped();
    test_comments_preserved();
    test_whitespace_preserved();
    test_placeholders();
    test_quoted_identifiers();
    test_create_table();
    test_blob_literal();
    test_line_column_tracking();
    test_tokenize_stripped();
    test_multiple_statements();
    test_join_query();
    test_empty_input();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
