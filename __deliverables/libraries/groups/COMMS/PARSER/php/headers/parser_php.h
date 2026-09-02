#pragma once
#include "ast_php.h"
#include "lexer_php.h"

namespace plphp {

class ParserPHP {
public:
    explicit ParserPHP(std::vector<TokenPHP> tokens);
    // Parses the whole token stream into a Program node. Throws std::runtime_error on syntax errors.
    NodePtr parse_program();

private:
    std::vector<TokenPHP> tokens_;
    size_t i_ = 0;

    const TokenPHP& peek(size_t offset = 0) const;
    const TokenPHP& advance();
    bool check_symbol(const std::string& sym) const;
    bool check_keyword(const std::string& kw) const;
    void expect_symbol(const std::string& sym);

    NodePtr parse_statement();
    NodePtr parse_block();
    NodePtr parse_statement_or_block();
    NodePtr parse_if();
    NodePtr parse_if_chain();
    NodePtr parse_while();
    NodePtr parse_for();
    NodePtr parse_foreach();
    NodePtr parse_function_decl();
    NodePtr parse_echo();

    NodePtr parse_expression();
    NodePtr parse_assignment();
    NodePtr parse_ternary();
    NodePtr parse_logical_or();
    NodePtr parse_logical_and();
    NodePtr parse_equality();
    NodePtr parse_comparison();
    NodePtr parse_concat();
    NodePtr parse_additive();
    NodePtr parse_multiplicative();
    NodePtr parse_unary();
    NodePtr parse_postfix();
    NodePtr parse_primary();
    NodePtr parse_array_literal();
};

} // namespace plphp
