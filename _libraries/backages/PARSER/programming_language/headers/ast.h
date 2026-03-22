#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace plang {

struct Node { virtual ~Node() = default; virtual void dump(int indent=0) const = 0; };

struct Expr : Node {};

struct IdentifierExpr : Expr { std::string name; void dump(int indent=0) const override; };
struct NumberExpr : Expr { std::string val; void dump(int indent=0) const override; };
struct BinaryExpr : Expr { std::string op; std::unique_ptr<Expr> left, right; void dump(int indent=0) const override; };

struct Stmt : Node {};
struct AssignStmt : Stmt { std::string target; std::unique_ptr<Expr> expr; void dump(int indent=0) const override; };
struct ExprStmt : Stmt { std::unique_ptr<Expr> expr; void dump(int indent=0) const override; };

struct FunctionDecl : Node { std::string name; std::vector<std::string> params; std::vector<std::unique_ptr<Stmt>> body; void dump(int indent=0) const override; };

struct Program : Node { std::vector<std::unique_ptr<Node>> items; void dump(int indent=0) const override; };

// helpers
inline void print_indent(int n) { for(int i=0;i<n;i++) std::cout<<"  "; }

} // namespace plang
