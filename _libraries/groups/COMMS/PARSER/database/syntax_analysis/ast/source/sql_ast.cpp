/**
 * @file sql_ast.cpp
 * @brief SQL AST generator implementation — recursive-descent parser
 *        that converts a token list into an AST.
 */

#include "../headers/sql_ast.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace ml {
namespace sql {
namespace ast {

// -----------------------------------------------------------------------
// Expression factories
// -----------------------------------------------------------------------
ExprPtr Expression::make_literal(const std::string& val) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::LITERAL;
    e->value = val;
    return e;
}

ExprPtr Expression::make_identifier(const std::string& val) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::IDENTIFIER;
    e->value = val;
    return e;
}

ExprPtr Expression::make_star() {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::STAR;
    e->value = "*";
    return e;
}

ExprPtr Expression::make_binary(const std::string& op, ExprPtr lhs, ExprPtr rhs) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::BINARY_OP;
    e->op    = op;
    e->left  = std::move(lhs);
    e->right = std::move(rhs);
    return e;
}

ExprPtr Expression::make_unary(const std::string& op, ExprPtr operand) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::UNARY_OP;
    e->op    = op;
    e->left  = std::move(operand);
    return e;
}

ExprPtr Expression::make_function(const std::string& name, std::vector<ExprPtr> args) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::FUNCTION_CALL;
    e->value = name;
    e->args  = std::move(args);
    return e;
}

ExprPtr Expression::make_placeholder(const std::string& val) {
    auto e = std::make_shared<Expression>();
    e->type  = ExprType::PLACEHOLDER;
    e->value = val;
    return e;
}

// -----------------------------------------------------------------------
// Helper: uppercase
// -----------------------------------------------------------------------
static std::string to_upper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return out;
}

// -----------------------------------------------------------------------
// Token access helpers
// -----------------------------------------------------------------------
static Token eof_token() {
    Token t;
    t.type = TokenType::END_OF_INPUT;
    return t;
}

const Token& SQLAstGenerator::current() const {
    if (pos_ < size_) return tokens_[pos_];
    static Token eof = eof_token();
    return eof;
}

const Token& SQLAstGenerator::peek(size_t ahead) const {
    size_t idx = pos_ + ahead;
    if (idx < size_) return tokens_[idx];
    static Token eof = eof_token();
    return eof;
}

const Token& SQLAstGenerator::advance() {
    const Token& tok = current();
    if (pos_ < size_) ++pos_;
    return tok;
}

bool SQLAstGenerator::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool SQLAstGenerator::match_keyword(const std::string& kw) {
    if (check_keyword(kw)) { advance(); return true; }
    return false;
}

bool SQLAstGenerator::check(TokenType type) const {
    return current().type == type;
}

bool SQLAstGenerator::check_keyword(const std::string& kw) const {
    return current().type == TokenType::KEYWORD && to_upper(current().value) == kw;
}

void SQLAstGenerator::expect(TokenType type, const std::string& msg) {
    if (!match(type)) {
        error_ = msg + " (got " + std::string(parser::token_type_name(current().type))
                 + " '" + current().value + "' at L"
                 + std::to_string(current().line) + ":"
                 + std::to_string(current().column) + ")";
        throw std::runtime_error(error_);
    }
}

void SQLAstGenerator::expect_keyword(const std::string& kw, const std::string& msg) {
    if (!match_keyword(kw)) {
        error_ = msg + " (got '" + current().value + "' at L"
                 + std::to_string(current().line) + ":"
                 + std::to_string(current().column) + ")";
        throw std::runtime_error(error_);
    }
}

bool SQLAstGenerator::at_end() const {
    return current().type == TokenType::END_OF_INPUT;
}

// -----------------------------------------------------------------------
// parse (entry points)
// -----------------------------------------------------------------------
std::vector<Statement> SQLAstGenerator::parse(const std::vector<Token>& tokens) {
    tokens_ = tokens.data();
    pos_    = 0;
    size_   = tokens.size();
    error_.clear();

    std::vector<Statement> stmts;

    while (!at_end()) {
        // Skip stray semicolons
        if (match(TokenType::SEMICOLON)) continue;
        stmts.push_back(parse_statement());
        // Consume optional trailing semicolon
        match(TokenType::SEMICOLON);
    }

    return stmts;
}

std::vector<Statement> SQLAstGenerator::parse(const std::string& sql) {
    parser::SQLTokenizer tokenizer;
    auto tokens = tokenizer.tokenize(sql);
    return parse(tokens);
}

// -----------------------------------------------------------------------
// parse_statement — dispatch on leading keyword
// -----------------------------------------------------------------------
Statement SQLAstGenerator::parse_statement() {
    Statement stmt;

    if (check_keyword("SELECT")) {
        stmt.type = StmtType::SELECT;
        stmt.node = parse_select();
    } else if (check_keyword("INSERT")) {
        stmt.type = StmtType::INSERT;
        stmt.node = parse_insert();
    } else if (check_keyword("UPDATE")) {
        stmt.type = StmtType::UPDATE;
        stmt.node = parse_update();
    } else if (check_keyword("DELETE")) {
        stmt.type = StmtType::DELETE_STMT;
        stmt.node = parse_delete();
    } else if (check_keyword("CREATE")) {
        stmt.type = StmtType::CREATE_TABLE;
        stmt.node = parse_create_table();
    } else if (check_keyword("DROP")) {
        stmt.type = StmtType::DROP_TABLE;
        stmt.node = parse_drop_table();
    } else {
        error_ = "Unexpected token '" + current().value + "' at start of statement";
        throw std::runtime_error(error_);
    }

    return stmt;
}

// -----------------------------------------------------------------------
// SELECT
// -----------------------------------------------------------------------
SelectStmt SQLAstGenerator::parse_select() {
    SelectStmt s;
    expect_keyword("SELECT", "Expected SELECT");

    // DISTINCT
    if (match_keyword("DISTINCT")) {
        s.distinct = true;
    }

    // Column list
    s.columns = parse_expression_list();

    // FROM
    if (match_keyword("FROM")) {
        if (check(TokenType::IDENTIFIER)) {
            s.from_table = advance().value;
        }
        // Optional alias
        if (match_keyword("AS")) {
            if (check(TokenType::IDENTIFIER)) s.from_alias = advance().value;
        } else if (check(TokenType::IDENTIFIER) &&
                   !check_keyword("WHERE") && !check_keyword("JOIN") &&
                   !check_keyword("INNER") && !check_keyword("LEFT") &&
                   !check_keyword("RIGHT") && !check_keyword("FULL") &&
                   !check_keyword("CROSS") && !check_keyword("NATURAL") &&
                   !check_keyword("ORDER") && !check_keyword("GROUP") &&
                   !check_keyword("HAVING") && !check_keyword("LIMIT") &&
                   !check_keyword("OFFSET") && !check_keyword("UNION")) {
            s.from_alias = advance().value;
        }

        // JOINs
        while (check_keyword("JOIN") || check_keyword("INNER") ||
               check_keyword("LEFT") || check_keyword("RIGHT") ||
               check_keyword("FULL") || check_keyword("CROSS") ||
               check_keyword("NATURAL")) {
            s.joins.push_back(parse_join());
        }
    }

    // WHERE
    if (match_keyword("WHERE")) {
        s.where_clause = parse_expression();
    }

    // GROUP BY
    if (match_keyword("GROUP")) {
        expect_keyword("BY", "Expected BY after GROUP");
        s.group_by = parse_expression_list();
    }

    // HAVING
    if (match_keyword("HAVING")) {
        s.having = parse_expression();
    }

    // ORDER BY
    if (match_keyword("ORDER")) {
        expect_keyword("BY", "Expected BY after ORDER");
        do {
            s.order_by.push_back(parse_order_by_item());
        } while (match(TokenType::COMMA));
    }

    // LIMIT
    if (match_keyword("LIMIT")) {
        if (check(TokenType::NUMERIC_LITERAL)) {
            s.limit = std::stoi(advance().value);
        }
    }

    // OFFSET
    if (match_keyword("OFFSET")) {
        if (check(TokenType::NUMERIC_LITERAL)) {
            s.offset = std::stoi(advance().value);
        }
    }

    return s;
}

// -----------------------------------------------------------------------
// INSERT
// -----------------------------------------------------------------------
InsertStmt SQLAstGenerator::parse_insert() {
    InsertStmt s;
    expect_keyword("INSERT", "Expected INSERT");
    expect_keyword("INTO", "Expected INTO");

    if (check(TokenType::IDENTIFIER)) {
        s.table = advance().value;
    }

    // Optional column list
    if (match(TokenType::OPEN_PAREN)) {
        while (!check(TokenType::CLOSE_PAREN) && !at_end()) {
            if (check(TokenType::IDENTIFIER)) {
                s.columns.push_back(advance().value);
            }
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::CLOSE_PAREN, "Expected )");
    }

    // VALUES
    expect_keyword("VALUES", "Expected VALUES");

    // One or more value rows
    do {
        expect(TokenType::OPEN_PAREN, "Expected (");
        std::vector<ExprPtr> row = parse_expression_list();
        expect(TokenType::CLOSE_PAREN, "Expected )");
        s.value_rows.push_back(std::move(row));
    } while (match(TokenType::COMMA));

    return s;
}

// -----------------------------------------------------------------------
// UPDATE
// -----------------------------------------------------------------------
UpdateStmt SQLAstGenerator::parse_update() {
    UpdateStmt s;
    expect_keyword("UPDATE", "Expected UPDATE");

    if (check(TokenType::IDENTIFIER)) {
        s.table = advance().value;
    }

    expect_keyword("SET", "Expected SET");

    // Assignments: col = expr [, col = expr …]
    do {
        std::string col;
        if (check(TokenType::IDENTIFIER)) {
            col = advance().value;
        }
        expect(TokenType::EQUALS, "Expected = in SET clause");
        ExprPtr val = parse_expression();
        s.assignments.emplace_back(std::move(col), std::move(val));
    } while (match(TokenType::COMMA));

    if (match_keyword("WHERE")) {
        s.where_clause = parse_expression();
    }

    return s;
}

// -----------------------------------------------------------------------
// DELETE
// -----------------------------------------------------------------------
DeleteStmt SQLAstGenerator::parse_delete() {
    DeleteStmt s;
    expect_keyword("DELETE", "Expected DELETE");
    expect_keyword("FROM", "Expected FROM");

    if (check(TokenType::IDENTIFIER)) {
        s.table = advance().value;
    }

    if (match_keyword("WHERE")) {
        s.where_clause = parse_expression();
    }

    return s;
}

// -----------------------------------------------------------------------
// CREATE TABLE
// -----------------------------------------------------------------------
CreateTableStmt SQLAstGenerator::parse_create_table() {
    CreateTableStmt s;
    expect_keyword("CREATE", "Expected CREATE");
    expect_keyword("TABLE", "Expected TABLE");

    // IF NOT EXISTS
    if (match_keyword("IF")) {
        expect_keyword("NOT", "Expected NOT after IF");
        expect_keyword("EXISTS", "Expected EXISTS");
        s.if_not_exists = true;
    }

    if (check(TokenType::IDENTIFIER)) {
        s.table = advance().value;
    }

    expect(TokenType::OPEN_PAREN, "Expected (");

    // Column definitions
    while (!check(TokenType::CLOSE_PAREN) && !at_end()) {
        s.columns.push_back(parse_column_def());
        if (!match(TokenType::COMMA)) break;
    }

    expect(TokenType::CLOSE_PAREN, "Expected )");

    return s;
}

// -----------------------------------------------------------------------
// DROP TABLE
// -----------------------------------------------------------------------
DropTableStmt SQLAstGenerator::parse_drop_table() {
    DropTableStmt s;
    expect_keyword("DROP", "Expected DROP");
    expect_keyword("TABLE", "Expected TABLE");

    if (match_keyword("IF")) {
        expect_keyword("EXISTS", "Expected EXISTS after IF");
        s.if_exists = true;
    }

    if (check(TokenType::IDENTIFIER)) {
        s.table = advance().value;
    }

    return s;
}

// -----------------------------------------------------------------------
// Expression parsers — precedence climbing
// -----------------------------------------------------------------------

ExprPtr SQLAstGenerator::parse_expression() {
    return parse_or();
}

ExprPtr SQLAstGenerator::parse_or() {
    auto left = parse_and();
    while (match_keyword("OR")) {
        auto right = parse_and();
        left = Expression::make_binary("OR", std::move(left), std::move(right));
    }
    return left;
}

ExprPtr SQLAstGenerator::parse_and() {
    auto left = parse_not();
    while (match_keyword("AND")) {
        auto right = parse_not();
        left = Expression::make_binary("AND", std::move(left), std::move(right));
    }
    return left;
}

ExprPtr SQLAstGenerator::parse_not() {
    if (match_keyword("NOT")) {
        auto operand = parse_not();
        return Expression::make_unary("NOT", std::move(operand));
    }
    return parse_comparison();
}

ExprPtr SQLAstGenerator::parse_comparison() {
    auto left = parse_addition();

    // IS [NOT] NULL
    if (check_keyword("IS")) {
        advance();
        bool neg = false;
        if (match_keyword("NOT")) neg = true;
        expect_keyword("NULL", "Expected NULL after IS");
        auto e = std::make_shared<Expression>();
        e->type    = ExprType::IS_NULL;
        e->left    = std::move(left);
        e->negated = neg;
        return e;
    }

    // BETWEEN … AND …
    if (check_keyword("BETWEEN")) {
        advance();
        auto low = parse_addition();
        expect_keyword("AND", "Expected AND in BETWEEN");
        auto high = parse_addition();
        auto e = std::make_shared<Expression>();
        e->type  = ExprType::BETWEEN;
        e->left  = std::move(low);
        e->right = std::move(high);
        e->args.push_back(std::move(left));  // the tested expression
        return e;
    }

    // [NOT] IN (…)
    if (check_keyword("NOT") && peek(1).type == TokenType::KEYWORD &&
        to_upper(peek(1).value) == "IN") {
        advance(); // NOT
        advance(); // IN
        expect(TokenType::OPEN_PAREN, "Expected ( after IN");
        auto vals = parse_expression_list();
        expect(TokenType::CLOSE_PAREN, "Expected )");
        auto e = std::make_shared<Expression>();
        e->type    = ExprType::IN_LIST;
        e->left    = std::move(left);
        e->args    = std::move(vals);
        e->negated = true;
        return e;
    }

    if (check_keyword("IN")) {
        advance();
        expect(TokenType::OPEN_PAREN, "Expected ( after IN");
        auto vals = parse_expression_list();
        expect(TokenType::CLOSE_PAREN, "Expected )");
        auto e = std::make_shared<Expression>();
        e->type = ExprType::IN_LIST;
        e->left = std::move(left);
        e->args = std::move(vals);
        return e;
    }

    // LIKE
    if (check_keyword("LIKE")) {
        advance();
        auto right = parse_addition();
        return Expression::make_binary("LIKE", std::move(left), std::move(right));
    }

    // Comparison operators
    if (check(TokenType::EQUALS) || check(TokenType::NOT_EQUALS) ||
        check(TokenType::LESS_THAN) || check(TokenType::GREATER_THAN) ||
        check(TokenType::LESS_EQUAL) || check(TokenType::GREATER_EQUAL)) {
        std::string op = advance().value;
        auto right = parse_addition();
        return Expression::make_binary(op, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr SQLAstGenerator::parse_addition() {
    auto left = parse_multiplication();
    while (check(TokenType::PLUS) || check(TokenType::MINUS) || check(TokenType::PIPE)) {
        std::string op = advance().value;
        auto right = parse_multiplication();
        left = Expression::make_binary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr SQLAstGenerator::parse_multiplication() {
    auto left = parse_unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = advance().value;
        auto right = parse_unary();
        left = Expression::make_binary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr SQLAstGenerator::parse_unary() {
    if (check(TokenType::MINUS)) {
        advance();
        auto operand = parse_unary();
        return Expression::make_unary("-", std::move(operand));
    }
    if (check(TokenType::PLUS)) {
        advance();
        return parse_unary();
    }
    return parse_primary();
}

ExprPtr SQLAstGenerator::parse_primary() {
    // NULL
    if (check_keyword("NULL")) {
        advance();
        return Expression::make_literal("NULL");
    }

    // TRUE / FALSE
    if (check_keyword("TRUE") || check_keyword("FALSE")) {
        return Expression::make_literal(advance().value);
    }

    // Numeric literal
    if (check(TokenType::NUMERIC_LITERAL)) {
        return Expression::make_literal(advance().value);
    }

    // String literal
    if (check(TokenType::STRING_LITERAL)) {
        return Expression::make_literal(advance().value);
    }

    // Placeholder
    if (check(TokenType::PLACEHOLDER)) {
        return Expression::make_placeholder(advance().value);
    }

    // Star
    if (check(TokenType::STAR)) {
        advance();
        auto expr = Expression::make_star();
        // Check for alias
        if (match_keyword("AS")) {
            if (check(TokenType::IDENTIFIER)) expr->alias = advance().value;
        }
        return expr;
    }

    // Parenthesized expression
    if (match(TokenType::OPEN_PAREN)) {
        auto expr = parse_expression();
        expect(TokenType::CLOSE_PAREN, "Expected )");
        return expr;
    }

    // Identifier — possibly qualified (table.column) or function call
    if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
        std::string name = advance().value;

        // Qualified name: table.column (column may shadow a keyword)
        if (match(TokenType::DOT)) {
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD) ||
                check(TokenType::STAR)) {
                std::string col = advance().value;
                name += "." + col;
            }
        }

        // Function call: name(…)
        if (check(TokenType::OPEN_PAREN)) {
            advance(); // (
            std::vector<ExprPtr> args;
            if (!check(TokenType::CLOSE_PAREN)) {
                // COUNT(*) special case
                if (check(TokenType::STAR)) {
                    args.push_back(Expression::make_star());
                    advance();
                } else {
                    args = parse_expression_list();
                }
            }
            expect(TokenType::CLOSE_PAREN, "Expected ) after function arguments");
            auto func = Expression::make_function(name, std::move(args));
            // Optional alias
            if (match_keyword("AS")) {
                if (check(TokenType::IDENTIFIER)) func->alias = advance().value;
            }
            return func;
        }

        auto expr = Expression::make_identifier(name);

        // Optional alias
        if (match_keyword("AS")) {
            if (check(TokenType::IDENTIFIER)) expr->alias = advance().value;
        }

        return expr;
    }

    error_ = "Unexpected token '" + current().value + "' in expression";
    throw std::runtime_error(error_);
}

// -----------------------------------------------------------------------
// parse_expression_list  — comma-separated expressions
// -----------------------------------------------------------------------
std::vector<ExprPtr> SQLAstGenerator::parse_expression_list() {
    std::vector<ExprPtr> list;
    list.push_back(parse_expression());
    while (match(TokenType::COMMA)) {
        list.push_back(parse_expression());
    }
    return list;
}

// -----------------------------------------------------------------------
// parse_column_def  — for CREATE TABLE
// -----------------------------------------------------------------------
ColumnDef SQLAstGenerator::parse_column_def() {
    ColumnDef col;

    // Column name
    if (check(TokenType::IDENTIFIER)) {
        col.name = advance().value;
    }

    // Type name (may be a keyword like INTEGER, TEXT, etc.)
    if (check(TokenType::KEYWORD) || check(TokenType::IDENTIFIER)) {
        col.type_name = advance().value;

        // Type with size: VARCHAR(255)
        if (match(TokenType::OPEN_PAREN)) {
            col.type_name += "(";
            while (!check(TokenType::CLOSE_PAREN) && !at_end()) {
                col.type_name += advance().value;
            }
            expect(TokenType::CLOSE_PAREN, "Expected ) in type spec");
            col.type_name += ")";
        }
    }

    // Constraints
    while (!check(TokenType::COMMA) && !check(TokenType::CLOSE_PAREN) && !at_end()) {
        if (match_keyword("PRIMARY")) {
            expect_keyword("KEY", "Expected KEY after PRIMARY");
            col.primary_key = true;
        } else if (match_keyword("AUTOINCREMENT")) {
            col.auto_increment = true;
        } else if (check_keyword("NOT")) {
            advance();
            expect_keyword("NULL", "Expected NULL after NOT");
            col.not_null = true;
        } else if (match_keyword("UNIQUE")) {
            col.unique = true;
        } else if (match_keyword("DEFAULT")) {
            // Grab the default value expression as raw text
            if (check(TokenType::NUMERIC_LITERAL) || check(TokenType::STRING_LITERAL)) {
                col.default_value = advance().value;
            } else if (check_keyword("NULL") || check_keyword("TRUE") || check_keyword("FALSE")) {
                col.default_value = advance().value;
            } else if (match(TokenType::OPEN_PAREN)) {
                // Expression default
                col.default_value = "(";
                int depth = 1;
                while (depth > 0 && !at_end()) {
                    if (check(TokenType::OPEN_PAREN)) ++depth;
                    if (check(TokenType::CLOSE_PAREN)) --depth;
                    if (depth > 0) col.default_value += advance().value;
                }
                expect(TokenType::CLOSE_PAREN, "Expected ) in DEFAULT");
                col.default_value += ")";
            } else {
                col.default_value = advance().value;
            }
        } else {
            // Skip unknown constraint tokens to avoid infinite loop
            advance();
        }
    }

    return col;
}

// -----------------------------------------------------------------------
// parse_join
// -----------------------------------------------------------------------
JoinClause SQLAstGenerator::parse_join() {
    JoinClause j;

    // Determine join type
    if (match_keyword("NATURAL")) {
        j.type = JoinType::NATURAL;
        match_keyword("JOIN");
    } else if (match_keyword("CROSS")) {
        j.type = JoinType::CROSS;
        match_keyword("JOIN");
    } else if (match_keyword("LEFT")) {
        j.type = JoinType::LEFT;
        match_keyword("OUTER");
        match_keyword("JOIN");
    } else if (match_keyword("RIGHT")) {
        j.type = JoinType::RIGHT;
        match_keyword("OUTER");
        match_keyword("JOIN");
    } else if (match_keyword("FULL")) {
        j.type = JoinType::FULL;
        match_keyword("OUTER");
        match_keyword("JOIN");
    } else if (match_keyword("INNER")) {
        j.type = JoinType::INNER;
        match_keyword("JOIN");
    } else if (match_keyword("JOIN")) {
        j.type = JoinType::INNER;
    }

    // Table name
    if (check(TokenType::IDENTIFIER)) {
        j.table = advance().value;
    }

    // Optional alias
    if (match_keyword("AS")) {
        if (check(TokenType::IDENTIFIER)) j.alias = advance().value;
    } else if (check(TokenType::IDENTIFIER) &&
               !check_keyword("ON") && !check_keyword("WHERE") &&
               !check_keyword("JOIN") && !check_keyword("INNER") &&
               !check_keyword("LEFT") && !check_keyword("RIGHT")) {
        j.alias = advance().value;
    }

    // ON condition
    if (match_keyword("ON")) {
        j.on_condition = parse_expression();
    }

    return j;
}

// -----------------------------------------------------------------------
// parse_order_by_item
// -----------------------------------------------------------------------
OrderByItem SQLAstGenerator::parse_order_by_item() {
    OrderByItem item;
    item.expr = parse_expression();
    item.ascending = true;

    if (match_keyword("ASC")) {
        item.ascending = true;
    } else if (match_keyword("DESC")) {
        item.ascending = false;
    }

    return item;
}

} // namespace ast
} // namespace sql
} // namespace ml
