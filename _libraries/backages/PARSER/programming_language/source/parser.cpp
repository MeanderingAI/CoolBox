#include "../headers/parser.h"
#include <stdexcept>

namespace plang {

Parser::Parser(const std::vector<Token>& tokens): tokens_(tokens) {}

const Token& Parser::peek() const { return tokens_[i_]; }
const Token& Parser::get() { return tokens_[i_++]; }

bool Parser::accept_keyword(const std::string &k) {
    if(peek().type==TokenType::Keyword && peek().text==k) { get(); return true; }
    return false;
}
bool Parser::accept_symbol(const std::string &s) {
    if(peek().type==TokenType::Symbol && peek().text==s) { get(); return true; }
    return false;
}

std::unique_ptr<Program> Parser::parse() {
    auto prog = std::make_unique<Program>();
    while(peek().type!=TokenType::End) {
        if(peek().type==TokenType::Newline) { get(); continue; }
        if(peek().type==TokenType::Keyword && peek().text=="function") {
            prog->items.push_back(parse_function());
            continue;
        }
        // fallback: try statement
        auto s = parse_statement();
        if(s) prog->items.push_back(std::move(s));
    }
    return prog;
}

std::unique_ptr<FunctionDecl> Parser::parse_function() {
    // function name(arg1,arg2)
    get(); // function
    if(peek().type!=TokenType::Identifier) throw std::runtime_error("Expected function name");
    auto fn = std::make_unique<FunctionDecl>();
    fn->name = get().text;
    accept_symbol("(");
    while(peek().type==TokenType::Identifier) {
        fn->params.push_back(get().text);
        if(!accept_symbol(",")) break;
    }
    accept_symbol(")");
    // body until 'end'
    while(!(peek().type==TokenType::Keyword && peek().text=="end") && peek().type!=TokenType::End) {
        if(peek().type==TokenType::Newline) { get(); continue; }
        fn->body.push_back(std::move(parse_statement()));
    }
    if(peek().type==TokenType::Keyword && peek().text=="end") get();
    return fn;
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    if(peek().type==TokenType::Identifier) return parse_assignment_or_expr();
    // other statements could be added here
    // consume and skip for now
    get();
    return nullptr;
}

std::unique_ptr<Stmt> Parser::parse_assignment_or_expr() {
    // simple lookahead for '='
    if(peek().type!=TokenType::Identifier) return nullptr;
    std::string name = get().text;
    if(peek().type==TokenType::Symbol && peek().text=="=") {
        get(); // =
        auto expr = parse_expression();
        // optionally consume newline or semicolon
        if(peek().type==TokenType::Newline) get();
        auto as = std::make_unique<AssignStmt>();
        as->target = name;
        as->expr = std::move(expr);
        return as;
    }
    // treat as expr stmt
    // (put back not implemented; simplify: create identifier expr)
    auto e = std::make_unique<IdentifierExpr>();
    e->name = name;
    if(peek().type==TokenType::Newline) get();
    auto es = std::make_unique<ExprStmt>();
    es->expr = std::move(e);
    return es;
}

std::unique_ptr<Expr> Parser::parse_expression() {
    // very simple: number or identifier or binary with single operator
    if(peek().type==TokenType::Number) {
        auto n = std::make_unique<NumberExpr>(); n->val = get().text; return n;
    }
    if(peek().type==TokenType::Identifier) {
        auto id = std::make_unique<IdentifierExpr>(); id->name = get().text; return id;
    }
    // fallback
    throw std::runtime_error("Unsupported expression");
}

// AST dump implementations
void IdentifierExpr::dump(int indent) const { print_indent(indent); std::cout<<"Identifier("<<name<<")\n"; }
void NumberExpr::dump(int indent) const { print_indent(indent); std::cout<<"Number("<<val<<")\n"; }
void BinaryExpr::dump(int indent) const { print_indent(indent); std::cout<<"Binary("<<op<<")\n"; if(left) left->dump(indent+1); if(right) right->dump(indent+1); }
void AssignStmt::dump(int indent) const { print_indent(indent); std::cout<<"Assign("<<target<<")\n"; if(expr) expr->dump(indent+1); }
void ExprStmt::dump(int indent) const { print_indent(indent); std::cout<<"ExprStmt\n"; if(expr) expr->dump(indent+1); }
void FunctionDecl::dump(int indent) const { print_indent(indent); std::cout<<"Function("<<name<<")\n"; print_indent(indent+1); std::cout<<"Params:"; for(auto &p:params) std::cout<<" "<<p; std::cout<<"\n"; print_indent(indent+1); std::cout<<"Body:\n"; for(auto &s:body) if(s) s->dump(indent+2); }
void Program::dump(int indent) const { print_indent(indent); std::cout<<"Program\n"; for(auto &i:items) if(i) i->dump(indent+1); }

} // namespace plang
