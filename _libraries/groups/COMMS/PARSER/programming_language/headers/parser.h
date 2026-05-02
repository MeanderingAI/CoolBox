#pragma once
#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>

namespace plang {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();
private:
    const std::vector<Token> tokens_;
    size_t i_ = 0;
    const Token& peek() const;
    const Token& get();
    bool accept_keyword(const std::string &k);
    bool accept_symbol(const std::string &s);
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<FunctionDecl> parse_function();
    std::unique_ptr<Stmt> parse_assignment_or_expr();
    std::unique_ptr<Expr> parse_expression();
};

} // namespace plang
