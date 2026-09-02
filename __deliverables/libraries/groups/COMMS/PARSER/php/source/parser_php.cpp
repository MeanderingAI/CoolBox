#include "parser_php.h"

#include <stdexcept>

namespace plphp {

namespace {
[[noreturn]] void fail(const std::string& msg, int line) {
    throw std::runtime_error("PHP parse error (line " + std::to_string(line) + "): " + msg);
}
} // namespace

ParserPHP::ParserPHP(std::vector<TokenPHP> tokens) : tokens_(std::move(tokens)) {}

const TokenPHP& ParserPHP::peek(size_t offset) const {
    const size_t idx = i_ + offset;
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
}

const TokenPHP& ParserPHP::advance() {
    const TokenPHP& tok = peek();
    if (i_ < tokens_.size() - 1) ++i_;
    return tok;
}

bool ParserPHP::check_symbol(const std::string& sym) const {
    return peek().type == TokenTypePHP::Symbol && peek().text == sym;
}

bool ParserPHP::check_keyword(const std::string& kw) const {
    return peek().type == TokenTypePHP::Keyword && peek().text == kw;
}

void ParserPHP::expect_symbol(const std::string& sym) {
    if (!check_symbol(sym)) {
        fail("expected '" + sym + "' but found '" + peek().text + "'", peek().line);
    }
    advance();
}

NodePtr ParserPHP::parse_program() {
    auto program = make_node(NodeType::Program);
    while (peek().type != TokenTypePHP::End) {
        program->children.push_back(parse_statement());
    }
    return program;
}

NodePtr ParserPHP::parse_block() {
    expect_symbol("{");
    auto block = make_node(NodeType::Block);
    while (!check_symbol("}") && peek().type != TokenTypePHP::End) {
        block->children.push_back(parse_statement());
    }
    expect_symbol("}");
    return block;
}

NodePtr ParserPHP::parse_statement_or_block() {
    if (check_symbol("{")) return parse_block();
    auto block = make_node(NodeType::Block);
    block->children.push_back(parse_statement());
    return block;
}

NodePtr ParserPHP::parse_statement() {
    const TokenPHP& tok = peek();

    if (tok.type == TokenTypePHP::InlineHtml) {
        auto node = make_node(NodeType::HtmlText);
        node->str_value = tok.text;
        node->line = tok.line;
        advance();
        return node;
    }

    if (tok.type == TokenTypePHP::Symbol && tok.text == "{") {
        return parse_block();
    }

    if (tok.type == TokenTypePHP::Keyword) {
        if (tok.text == "echo" || tok.text == "print") return parse_echo();
        if (tok.text == "if") return parse_if();
        if (tok.text == "while") return parse_while();
        if (tok.text == "for") return parse_for();
        if (tok.text == "foreach") return parse_foreach();
        if (tok.text == "function") return parse_function_decl();

        if (tok.text == "return") {
            advance();
            auto node = make_node(NodeType::ReturnStmt);
            node->line = tok.line;
            if (!check_symbol(";")) node->children.push_back(parse_expression());
            expect_symbol(";");
            return node;
        }
        if (tok.text == "break") {
            advance();
            expect_symbol(";");
            return make_node(NodeType::BreakStmt);
        }
        if (tok.text == "continue") {
            advance();
            expect_symbol(";");
            return make_node(NodeType::ContinueStmt);
        }
        if (tok.text == "global") {
            advance();
            auto node = make_node(NodeType::GlobalStmt);
            while (true) {
                if (peek().type != TokenTypePHP::Variable) fail("expected variable after 'global'", peek().line);
                auto var = make_node(NodeType::Variable);
                var->str_value = advance().text;
                node->children.push_back(var);
                if (check_symbol(",")) { advance(); continue; }
                break;
            }
            expect_symbol(";");
            return node;
        }
    }

    if (check_symbol(";")) {
        advance();
        return make_node(NodeType::Block);
    }

    auto expr = parse_expression();
    auto stmt = make_node(NodeType::ExprStmt);
    stmt->children.push_back(expr);
    expect_symbol(";");
    return stmt;
}

NodePtr ParserPHP::parse_echo() {
    const int line = peek().line;
    advance(); // echo/print
    auto node = make_node(NodeType::EchoStmt);
    node->line = line;
    node->children.push_back(parse_expression());
    while (check_symbol(",")) {
        advance();
        node->children.push_back(parse_expression());
    }
    expect_symbol(";");
    return node;
}

NodePtr ParserPHP::parse_if() {
    return parse_if_chain();
}

// Handles both 'if (...)' and 'elseif (...)' since both start with a
// condition-parenthesis-body shape; the leading keyword is consumed here.
NodePtr ParserPHP::parse_if_chain() {
    advance(); // 'if' or 'elseif'
    expect_symbol("(");
    auto cond = parse_expression();
    expect_symbol(")");
    auto then_branch = parse_statement_or_block();

    auto node = make_node(NodeType::IfStmt);
    node->children = {cond, then_branch};

    if (check_keyword("elseif")) {
        node->children.push_back(parse_if_chain());
    } else if (check_keyword("else")) {
        advance();
        node->children.push_back(check_keyword("if") ? parse_if_chain() : parse_statement_or_block());
    }
    return node;
}

NodePtr ParserPHP::parse_while() {
    advance(); // while
    expect_symbol("(");
    auto cond = parse_expression();
    expect_symbol(")");
    auto body = parse_statement_or_block();

    auto node = make_node(NodeType::WhileStmt);
    node->children = {cond, body};
    return node;
}

NodePtr ParserPHP::parse_for() {
    advance(); // for
    expect_symbol("(");
    NodePtr init = check_symbol(";") ? nullptr : parse_expression();
    expect_symbol(";");
    NodePtr cond = check_symbol(";") ? nullptr : parse_expression();
    expect_symbol(";");
    NodePtr post = check_symbol(")") ? nullptr : parse_expression();
    expect_symbol(")");
    auto body = parse_statement_or_block();

    auto node = make_node(NodeType::ForStmt);
    auto wrap = [](NodePtr n) { return n ? n : make_node(NodeType::NullLiteral); };
    node->children = {wrap(init), wrap(cond), wrap(post), body};
    node->bool_value = (init != nullptr);       // reused as "has init" flag
    node->interpolate = (cond != nullptr);      // reused as "has cond" flag
    node->double_value = (post != nullptr) ? 1.0 : 0.0; // reused as "has post" flag
    return node;
}

NodePtr ParserPHP::parse_foreach() {
    advance(); // foreach
    expect_symbol("(");
    auto array_expr = parse_expression();
    if (!check_keyword("as")) fail("expected 'as' in foreach", peek().line);
    advance();

    if (peek().type != TokenTypePHP::Variable) fail("expected variable in foreach", peek().line);
    auto first_var = make_node(NodeType::Variable);
    first_var->str_value = advance().text;

    NodePtr key_var;
    NodePtr value_var = first_var;
    if (check_symbol("=>")) {
        advance();
        if (peek().type != TokenTypePHP::Variable) fail("expected variable after '=>' in foreach", peek().line);
        key_var = first_var;
        value_var = make_node(NodeType::Variable);
        value_var->str_value = advance().text;
    }
    expect_symbol(")");
    auto body = parse_statement_or_block();

    auto node = make_node(NodeType::ForeachStmt);
    node->children = {array_expr, key_var ? key_var : make_node(NodeType::NullLiteral), value_var, body};
    node->bool_value = (key_var != nullptr); // has explicit key
    return node;
}

NodePtr ParserPHP::parse_function_decl() {
    advance(); // function
    if (peek().type != TokenTypePHP::Identifier) fail("expected function name", peek().line);
    auto node = make_node(NodeType::FunctionDecl);
    node->str_value = advance().text;

    expect_symbol("(");
    auto params = make_node(NodeType::Block);
    while (!check_symbol(")")) {
        if (peek().type != TokenTypePHP::Variable) fail("expected parameter", peek().line);
        auto param = make_node(NodeType::Variable);
        param->str_value = advance().text;
        if (check_symbol("=")) {
            advance();
            param->children.push_back(parse_ternary());
        }
        params->children.push_back(param);
        if (check_symbol(",")) { advance(); continue; }
        break;
    }
    expect_symbol(")");
    auto body = parse_block();

    node->children = {params, body};
    return node;
}

NodePtr ParserPHP::parse_expression() {
    return parse_assignment();
}

NodePtr ParserPHP::parse_assignment() {
    auto left = parse_ternary();

    static const std::vector<std::pair<std::string, std::string>> compound_ops = {
        {"+=", "+"}, {"-=", "-"}, {".=", "."}, {"*=", "*"}, {"/=", "/"}, {"%=", "%"}
    };

    if (check_symbol("=")) {
        advance();
        auto rhs = parse_assignment();
        auto node = make_node(NodeType::Assign);
        node->children = {left, rhs};
        return node;
    }

    for (const auto& [sym, op] : compound_ops) {
        if (check_symbol(sym)) {
            advance();
            auto rhs = parse_assignment();
            auto node = make_node(NodeType::CompoundAssign);
            node->str_value = op;
            node->children = {left, rhs};
            return node;
        }
    }

    return left;
}

NodePtr ParserPHP::parse_ternary() {
    auto cond = parse_logical_or();
    if (check_symbol("?")) {
        advance();
        NodePtr then_expr;
        if (!check_symbol(":")) then_expr = parse_expression();
        expect_symbol(":");
        auto else_expr = parse_expression();
        auto node = make_node(NodeType::Ternary);
        node->children = {cond, then_expr ? then_expr : cond, else_expr};
        node->bool_value = (then_expr == nullptr); // short-hand ?: form
        return node;
    }
    return cond;
}

NodePtr ParserPHP::parse_logical_or() {
    auto left = parse_logical_and();
    while (check_symbol("||") || check_keyword("or")) {
        advance();
        auto right = parse_logical_and();
        auto node = make_node(NodeType::LogicalOr);
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_logical_and() {
    auto left = parse_equality();
    while (check_symbol("&&") || check_keyword("and")) {
        advance();
        auto right = parse_equality();
        auto node = make_node(NodeType::LogicalAnd);
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_equality() {
    auto left = parse_comparison();
    while (check_symbol("==") || check_symbol("!=") || check_symbol("===") || check_symbol("!==")) {
        const std::string op = advance().text;
        auto right = parse_comparison();
        auto node = make_node(NodeType::BinaryOp);
        node->str_value = op;
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_comparison() {
    auto left = parse_concat();
    while (check_symbol("<") || check_symbol(">") || check_symbol("<=") || check_symbol(">=")) {
        const std::string op = advance().text;
        auto right = parse_concat();
        auto node = make_node(NodeType::BinaryOp);
        node->str_value = op;
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_concat() {
    auto left = parse_additive();
    while (check_symbol(".")) {
        advance();
        auto right = parse_additive();
        auto node = make_node(NodeType::BinaryOp);
        node->str_value = ".";
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_additive() {
    auto left = parse_multiplicative();
    while (check_symbol("+") || check_symbol("-")) {
        const std::string op = advance().text;
        auto right = parse_multiplicative();
        auto node = make_node(NodeType::BinaryOp);
        node->str_value = op;
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_multiplicative() {
    auto left = parse_unary();
    while (check_symbol("*") || check_symbol("/") || check_symbol("%")) {
        const std::string op = advance().text;
        auto right = parse_unary();
        auto node = make_node(NodeType::BinaryOp);
        node->str_value = op;
        node->children = {left, right};
        left = node;
    }
    return left;
}

NodePtr ParserPHP::parse_unary() {
    if (check_symbol("!")) {
        advance();
        auto node = make_node(NodeType::UnaryNot);
        node->children = {parse_unary()};
        return node;
    }
    if (check_symbol("-")) {
        advance();
        auto node = make_node(NodeType::UnaryMinus);
        node->children = {parse_unary()};
        return node;
    }
    if (check_symbol("+")) {
        advance();
        return parse_unary();
    }
    return parse_postfix();
}

NodePtr ParserPHP::parse_postfix() {
    auto node = parse_primary();
    while (true) {
        if (check_symbol("[")) {
            advance();
            NodePtr index_expr = check_symbol("]") ? nullptr : parse_expression();
            expect_symbol("]");
            auto access = make_node(NodeType::ArrayAccess);
            access->children = {node, index_expr ? index_expr : make_node(NodeType::NullLiteral)};
            access->bool_value = (index_expr == nullptr); // append form: $arr[] = ...
            node = access;
            continue;
        }
        break;
    }
    return node;
}

NodePtr ParserPHP::parse_array_literal() {
    const bool bracket_form = check_symbol("[");
    advance(); // '[' or 'array('s '('... handled by caller for 'array'
    const std::string close = bracket_form ? "]" : ")";

    auto node = make_node(NodeType::ArrayLiteral);
    while (!check_symbol(close)) {
        auto first = parse_ternary();
        if (check_symbol("=>")) {
            advance();
            auto value = parse_ternary();
            auto pair = make_node(NodeType::KeyValue);
            pair->children = {first, value};
            node->children.push_back(pair);
        } else {
            node->children.push_back(first);
        }
        if (check_symbol(",")) { advance(); continue; }
        break;
    }
    expect_symbol(close);
    return node;
}

NodePtr ParserPHP::parse_primary() {
    const TokenPHP& tok = peek();

    if (tok.type == TokenTypePHP::Variable) {
        advance();
        auto node = make_node(NodeType::Variable);
        node->str_value = tok.text;
        node->line = tok.line;
        return node;
    }

    if (tok.type == TokenTypePHP::Int) {
        advance();
        auto node = make_node(NodeType::IntLiteral);
        node->int_value = std::stoll(tok.text);
        return node;
    }

    if (tok.type == TokenTypePHP::Double) {
        advance();
        auto node = make_node(NodeType::DoubleLiteral);
        node->double_value = std::stod(tok.text);
        return node;
    }

    if (tok.type == TokenTypePHP::String) {
        advance();
        auto node = make_node(NodeType::StringLiteral);
        node->str_value = tok.text;
        node->interpolate = tok.interpolate;
        return node;
    }

    if (tok.type == TokenTypePHP::Keyword) {
        if (tok.text == "true" || tok.text == "false") {
            advance();
            auto node = make_node(NodeType::BoolLiteral);
            node->bool_value = (tok.text == "true");
            return node;
        }
        if (tok.text == "null") {
            advance();
            return make_node(NodeType::NullLiteral);
        }
        if (tok.text == "array") {
            advance();
            expect_symbol("(");
            auto node = make_node(NodeType::ArrayLiteral);
            while (!check_symbol(")")) {
                auto first = parse_ternary();
                if (check_symbol("=>")) {
                    advance();
                    auto value = parse_ternary();
                    auto pair = make_node(NodeType::KeyValue);
                    pair->children = {first, value};
                    node->children.push_back(pair);
                } else {
                    node->children.push_back(first);
                }
                if (check_symbol(",")) { advance(); continue; }
                break;
            }
            expect_symbol(")");
            return node;
        }
    }

    if (check_symbol("[")) {
        return parse_array_literal();
    }

    if (check_symbol("(")) {
        advance();
        auto expr = parse_expression();
        expect_symbol(")");
        return expr;
    }

    if (tok.type == TokenTypePHP::Identifier) {
        advance();
        if (check_symbol("(")) {
            advance();
            auto node = make_node(NodeType::FunctionCall);
            node->str_value = tok.text;
            node->line = tok.line;
            while (!check_symbol(")")) {
                node->children.push_back(parse_expression());
                if (check_symbol(",")) { advance(); continue; }
                break;
            }
            expect_symbol(")");
            return node;
        }
        auto node = make_node(NodeType::Identifier);
        node->str_value = tok.text;
        return node;
    }

    fail("unexpected token '" + tok.text + "'", tok.line);
}

} // namespace plphp
