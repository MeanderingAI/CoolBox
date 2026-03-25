/**
 * @file sql_ast.h
 * @brief SQL Abstract Syntax Tree — node types and parser that converts
 *        a token list (from sql_tokenizer) into an AST.
 *
 * Supports: SELECT, INSERT, UPDATE, DELETE, CREATE TABLE, DROP TABLE.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../../../lexical_analysis/parser/headers/sql_tokenizer.h"

namespace ml {
namespace sql {
namespace ast {

using Token     = parser::Token;
using TokenType = parser::TokenType;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
struct Expression;
using ExprPtr = std::shared_ptr<Expression>;

// -----------------------------------------------------------------------
// Expressions
// -----------------------------------------------------------------------

enum class ExprType {
    LITERAL,          // 42, 'hello', TRUE, NULL
    IDENTIFIER,       // column or table.column
    BINARY_OP,        // left OP right
    UNARY_OP,         // NOT expr, -expr
    FUNCTION_CALL,    // COUNT(*), MAX(x)
    STAR,             // *
    PLACEHOLDER,      // ?, :name, $1
    BETWEEN,          // expr BETWEEN low AND high
    IN_LIST,          // expr IN (v1, v2, …)
    IS_NULL,          // expr IS [NOT] NULL
    SUBQUERY,         // (SELECT …)  — placeholder for nested selects
};

struct Expression {
    ExprType type;

    // LITERAL / IDENTIFIER / STAR / PLACEHOLDER
    std::string value;

    // BINARY_OP / UNARY_OP operator text (e.g. "+", "AND", "NOT")
    std::string op;

    // Children
    ExprPtr left;
    ExprPtr right;

    // FUNCTION_CALL arguments / IN_LIST values
    std::vector<ExprPtr> args;

    // BETWEEN low/high (reuses left=low, right=high)

    // IS_NULL negated?
    bool negated = false;

    // Optional alias (AS …)
    std::string alias;

    // Convenience factories
    static ExprPtr make_literal(const std::string& val);
    static ExprPtr make_identifier(const std::string& val);
    static ExprPtr make_star();
    static ExprPtr make_binary(const std::string& op, ExprPtr lhs, ExprPtr rhs);
    static ExprPtr make_unary(const std::string& op, ExprPtr operand);
    static ExprPtr make_function(const std::string& name, std::vector<ExprPtr> args);
    static ExprPtr make_placeholder(const std::string& val);
};

// -----------------------------------------------------------------------
// ORDER BY item
// -----------------------------------------------------------------------
struct OrderByItem {
    ExprPtr  expr;
    bool     ascending = true;   // ASC by default
};

// -----------------------------------------------------------------------
// JOIN clause
// -----------------------------------------------------------------------
enum class JoinType {
    INNER,
    LEFT,
    RIGHT,
    FULL,
    CROSS,
    NATURAL,
};

struct JoinClause {
    JoinType    type = JoinType::INNER;
    std::string table;
    std::string alias;
    ExprPtr     on_condition;  // ON …
};

// -----------------------------------------------------------------------
// Column definition (for CREATE TABLE)
// -----------------------------------------------------------------------
struct ColumnDef {
    std::string name;
    std::string type_name;          // e.g. "INTEGER", "VARCHAR(255)"
    bool primary_key    = false;
    bool auto_increment = false;
    bool not_null       = false;
    bool unique         = false;
    std::string default_value;      // raw SQL text, empty if none
};

// -----------------------------------------------------------------------
// Statement nodes
// -----------------------------------------------------------------------

enum class StmtType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE_STMT,
    CREATE_TABLE,
    DROP_TABLE,
};

// ---------- SELECT ----------
struct SelectStmt {
    bool distinct = false;
    std::vector<ExprPtr>     columns;       // result columns / expressions
    std::string              from_table;
    std::string              from_alias;
    std::vector<JoinClause>  joins;
    ExprPtr                  where_clause;
    std::vector<ExprPtr>     group_by;
    ExprPtr                  having;
    std::vector<OrderByItem> order_by;
    std::optional<int>       limit;
    std::optional<int>       offset;
};

// ---------- INSERT ----------
struct InsertStmt {
    std::string              table;
    std::vector<std::string> columns;       // may be empty (INSERT INTO t VALUES …)
    std::vector<std::vector<ExprPtr>> value_rows;  // one or more rows
};

// ---------- UPDATE ----------
struct UpdateStmt {
    std::string                             table;
    std::vector<std::pair<std::string, ExprPtr>> assignments; // SET col = expr
    ExprPtr                                 where_clause;
};

// ---------- DELETE ----------
struct DeleteStmt {
    std::string table;
    ExprPtr     where_clause;
};

// ---------- CREATE TABLE ----------
struct CreateTableStmt {
    std::string              table;
    bool                     if_not_exists = false;
    std::vector<ColumnDef>   columns;
};

// ---------- DROP TABLE ----------
struct DropTableStmt {
    std::string table;
    bool        if_exists = false;
};

// -----------------------------------------------------------------------
// Generic Statement wrapper
// -----------------------------------------------------------------------
using StmtVariant = std::variant<
    SelectStmt,
    InsertStmt,
    UpdateStmt,
    DeleteStmt,
    CreateTableStmt,
    DropTableStmt
>;

struct Statement {
    StmtType    type;
    StmtVariant node;
};

// -----------------------------------------------------------------------
// SQL AST Generator  —  parses a token stream into a list of Statements
// -----------------------------------------------------------------------
class SQLAstGenerator {
public:
    /// Parse a token list (as produced by SQLTokenizer::tokenize) into statements.
    std::vector<Statement> parse(const std::vector<Token>& tokens);

    /// Convenience: tokenize + parse in one call.
    std::vector<Statement> parse(const std::string& sql);

    /// Last error message (empty on success).
    const std::string& error() const noexcept { return error_; }

private:
    const Token* tokens_ = nullptr;
    size_t       pos_    = 0;
    size_t       size_   = 0;
    std::string  error_;

    // ---- token access helpers ------------------------------------------
    const Token& current() const;
    const Token& peek(size_t ahead = 0) const;
    const Token& advance();
    bool         match(TokenType type);
    bool         match_keyword(const std::string& kw);
    bool         check(TokenType type) const;
    bool         check_keyword(const std::string& kw) const;
    void         expect(TokenType type, const std::string& msg);
    void         expect_keyword(const std::string& kw, const std::string& msg);
    bool         at_end() const;

    // ---- statement parsers ---------------------------------------------
    Statement parse_statement();
    SelectStmt      parse_select();
    InsertStmt      parse_insert();
    UpdateStmt      parse_update();
    DeleteStmt      parse_delete();
    CreateTableStmt parse_create_table();
    DropTableStmt   parse_drop_table();

    // ---- expression parsers (precedence climbing) ----------------------
    ExprPtr parse_expression();
    ExprPtr parse_or();
    ExprPtr parse_and();
    ExprPtr parse_not();
    ExprPtr parse_comparison();
    ExprPtr parse_addition();
    ExprPtr parse_multiplication();
    ExprPtr parse_unary();
    ExprPtr parse_primary();

    // ---- helpers -------------------------------------------------------
    std::vector<ExprPtr> parse_expression_list();
    ColumnDef            parse_column_def();
    JoinClause           parse_join();
    OrderByItem          parse_order_by_item();
};

} // namespace ast
} // namespace sql
} // namespace ml
