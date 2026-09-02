#pragma once
#include <memory>
#include <string>
#include <vector>

namespace plphp {

enum class NodeType {
    Program, HtmlText, EchoStmt, ExprStmt, IfStmt, WhileStmt, ForStmt, ForeachStmt,
    Block, FunctionDecl, ReturnStmt, BreakStmt, ContinueStmt, GlobalStmt,
    Assign, CompoundAssign, BinaryOp, UnaryNot, UnaryMinus, LogicalAnd, LogicalOr, Ternary,
    Variable, IntLiteral, DoubleLiteral, StringLiteral, BoolLiteral, NullLiteral,
    ArrayLiteral, ArrayAccess, FunctionCall, Identifier, KeyValue
};

struct Node {
    NodeType type;
    std::string str_value;      // identifier / variable name / operator / string content
    long long int_value = 0;
    double double_value = 0.0;
    bool bool_value = false;
    bool interpolate = false;   // string literal came from double quotes
    std::vector<std::shared_ptr<Node>> children;
    int line = 0;
};

using NodePtr = std::shared_ptr<Node>;

inline NodePtr make_node(NodeType type) {
    auto node = std::make_shared<Node>();
    node->type = type;
    return node;
}

} // namespace plphp
