#pragma once
#include "ast_php.h"
#include "value_php.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace plphp {

// Tree-walking evaluator for the PHP subset produced by ParserPHP.
// Executes a Program node and returns the text written by echo/print
// and any literal HTML segments, in source order.
class InterpreterPHP {
public:
    InterpreterPHP();

    void set_get_params(const std::map<std::string, std::string>& params);
    void set_post_params(const std::map<std::string, std::string>& params);
    void set_server_params(const std::map<std::string, std::string>& params);

    // Throws std::runtime_error on PHP-level errors (undefined function, etc.).
    std::string run(const NodePtr& program);

private:
    using Scope = std::unordered_map<std::string, Value>;

    enum class FlowKind { None, Return, Break, Continue };
    struct Flow {
        FlowKind kind = FlowKind::None;
        Value return_value;
    };

    std::string output_;
    std::unordered_map<std::string, NodePtr> functions_;
    std::vector<Scope> scopes_;
    std::vector<std::set<std::string>> scope_globals_; // names aliased via 'global', parallel to scopes_
    Scope globals_;

    Scope& current_scope();
    Value get_variable(const std::string& name);
    void assign_variable(const std::string& name, const Value& value);
    Value* lvalue_ref(const NodePtr& node, bool create_if_missing);

    void exec_block(const NodePtr& block, Flow& flow);
    void exec_statement(const NodePtr& node, Flow& flow);

    Value eval(const NodePtr& node);
    Value eval_binary(const std::string& op, const Value& lhs, const Value& rhs);
    std::string interpolate_string(const std::string& raw);

    Value call_function(const std::string& name, std::vector<Value>& args);
    bool call_builtin(const std::string& name, std::vector<Value>& args, Value& result);
};

} // namespace plphp
