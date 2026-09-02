#include "interpreter_php.h"

#include <cctype>
#include <stdexcept>

namespace plphp {

namespace {

std::string print_r_string(const Value& v, int indent) {
    if (v.type != ValueType::Array) return v.to_string();
    std::string out = "Array\n" + std::string(static_cast<size_t>(indent), ' ') + "(\n";
    if (v.arr) {
        for (const auto& kv : *v.arr) {
            out += std::string(static_cast<size_t>(indent + 4), ' ') + "[" + kv.first.to_string() + "] => " +
                   print_r_string(kv.second, indent + 8) + "\n";
        }
    }
    out += std::string(static_cast<size_t>(indent), ' ') + ")\n";
    return out;
}

} // namespace

InterpreterPHP::InterpreterPHP() {
    globals_["_GET"] = Value::array_value();
    globals_["_POST"] = Value::array_value();
    globals_["_SERVER"] = Value::array_value();
}

void InterpreterPHP::set_get_params(const std::map<std::string, std::string>& params) {
    Value arr = Value::array_value();
    for (const auto& [key, value] : params) arr.set(Value::string_value(key), Value::string_value(value));
    globals_["_GET"] = arr;
}

void InterpreterPHP::set_post_params(const std::map<std::string, std::string>& params) {
    Value arr = Value::array_value();
    for (const auto& [key, value] : params) arr.set(Value::string_value(key), Value::string_value(value));
    globals_["_POST"] = arr;
}

void InterpreterPHP::set_server_params(const std::map<std::string, std::string>& params) {
    Value arr = Value::array_value();
    for (const auto& [key, value] : params) arr.set(Value::string_value(key), Value::string_value(value));
    globals_["_SERVER"] = arr;
}

InterpreterPHP::Scope& InterpreterPHP::current_scope() {
    return scopes_.empty() ? globals_ : scopes_.back();
}

Value InterpreterPHP::get_variable(const std::string& name) {
    if (!scopes_.empty() && scope_globals_.back().count(name) != 0) {
        auto it = globals_.find(name);
        return it != globals_.end() ? it->second : Value::null_value();
    }
    Scope& scope = current_scope();
    auto it = scope.find(name);
    return it != scope.end() ? it->second : Value::null_value();
}

void InterpreterPHP::assign_variable(const std::string& name, const Value& value) {
    if (!scopes_.empty() && scope_globals_.back().count(name) != 0) {
        globals_[name] = value;
        return;
    }
    current_scope()[name] = value;
}

Value* InterpreterPHP::lvalue_ref(const NodePtr& node, bool create_if_missing) {
    if (node->type == NodeType::Variable) {
        Scope* target = &current_scope();
        if (!scopes_.empty() && scope_globals_.back().count(node->str_value) != 0) {
            target = &globals_;
        }
        return &(*target)[node->str_value];
    }

    if (node->type == NodeType::ArrayAccess) {
        Value* base = lvalue_ref(node->children[0], create_if_missing);
        if (!base) return nullptr;
        if (base->type != ValueType::Array) {
            if (!create_if_missing) return nullptr;
            *base = Value::array_value();
        }
        if (node->bool_value) { // append form: $arr[] = ...
            base->push(Value::null_value());
            return &base->arr->back().second;
        }
        Value key = eval(node->children[1]);
        Value* found = base->find(key);
        if (found) return found;
        base->set(key, Value::null_value());
        return base->find(key);
    }

    throw std::runtime_error("invalid assignment target");
}

void InterpreterPHP::exec_block(const NodePtr& block, Flow& flow) {
    for (const auto& stmt : block->children) {
        exec_statement(stmt, flow);
        if (flow.kind != FlowKind::None) return;
    }
}

void InterpreterPHP::exec_statement(const NodePtr& node, Flow& flow) {
    switch (node->type) {
        case NodeType::HtmlText:
            output_ += node->str_value;
            return;

        case NodeType::EchoStmt:
            for (const auto& child : node->children) {
                Value v = eval(child);
                output_ += v.type == ValueType::Array ? print_r_string(v, 0) : v.to_string();
            }
            return;

        case NodeType::ExprStmt:
            eval(node->children[0]);
            return;

        case NodeType::Block:
            exec_block(node, flow);
            return;

        case NodeType::IfStmt:
            if (eval(node->children[0]).to_bool()) {
                exec_statement(node->children[1], flow);
            } else if (node->children.size() > 2) {
                exec_statement(node->children[2], flow);
            }
            return;

        case NodeType::WhileStmt:
            while (eval(node->children[0]).to_bool()) {
                exec_statement(node->children[1], flow);
                if (flow.kind == FlowKind::Break) { flow.kind = FlowKind::None; break; }
                if (flow.kind == FlowKind::Return) return;
                if (flow.kind == FlowKind::Continue) flow.kind = FlowKind::None;
            }
            return;

        case NodeType::ForStmt: {
            const bool has_init = node->bool_value;
            const bool has_cond = node->interpolate;
            const bool has_post = node->double_value != 0.0;
            if (has_init) eval(node->children[0]);
            while (!has_cond || eval(node->children[1]).to_bool()) {
                exec_statement(node->children[3], flow);
                if (flow.kind == FlowKind::Break) { flow.kind = FlowKind::None; break; }
                if (flow.kind == FlowKind::Return) return;
                if (flow.kind == FlowKind::Continue) flow.kind = FlowKind::None;
                if (has_post) eval(node->children[2]);
            }
            return;
        }

        case NodeType::ForeachStmt: {
            Value array_val = eval(node->children[0]);
            if (!array_val.is_array() || !array_val.arr) return;
            const bool has_key = node->bool_value;
            const auto snapshot = *array_val.arr;
            for (const auto& kv : snapshot) {
                if (has_key) assign_variable(node->children[1]->str_value, kv.first);
                assign_variable(node->children[2]->str_value, clone(kv.second));
                exec_statement(node->children[3], flow);
                if (flow.kind == FlowKind::Break) { flow.kind = FlowKind::None; break; }
                if (flow.kind == FlowKind::Return) return;
                if (flow.kind == FlowKind::Continue) flow.kind = FlowKind::None;
            }
            return;
        }

        case NodeType::FunctionDecl:
            functions_[node->str_value] = node;
            return;

        case NodeType::ReturnStmt:
            flow.kind = FlowKind::Return;
            flow.return_value = node->children.empty() ? Value::null_value() : eval(node->children[0]);
            return;

        case NodeType::BreakStmt:
            flow.kind = FlowKind::Break;
            return;

        case NodeType::ContinueStmt:
            flow.kind = FlowKind::Continue;
            return;

        case NodeType::GlobalStmt:
            if (!scopes_.empty()) {
                for (const auto& var : node->children) scope_globals_.back().insert(var->str_value);
            }
            return;

        default:
            eval(node);
            return;
    }
}

Value InterpreterPHP::eval_binary(const std::string& op, const Value& lhs, const Value& rhs) {
    if (op == ".") return Value::string_value(lhs.to_string() + rhs.to_string());
    if (op == "==") return Value::bool_value(lhs.loose_equals(rhs));
    if (op == "!=") return Value::bool_value(!lhs.loose_equals(rhs));
    if (op == "===") return Value::bool_value(lhs.type == rhs.type && lhs.loose_equals(rhs));
    if (op == "!==") return Value::bool_value(!(lhs.type == rhs.type && lhs.loose_equals(rhs)));
    if (op == "<") return Value::bool_value(lhs.to_double() < rhs.to_double());
    if (op == ">") return Value::bool_value(lhs.to_double() > rhs.to_double());
    if (op == "<=") return Value::bool_value(lhs.to_double() <= rhs.to_double());
    if (op == ">=") return Value::bool_value(lhs.to_double() >= rhs.to_double());

    const bool both_int = lhs.type == ValueType::Int && rhs.type == ValueType::Int;
    if (op == "+") return both_int ? Value::int_value(lhs.i + rhs.i) : Value::double_value(lhs.to_double() + rhs.to_double());
    if (op == "-") return both_int ? Value::int_value(lhs.i - rhs.i) : Value::double_value(lhs.to_double() - rhs.to_double());
    if (op == "*") return both_int ? Value::int_value(lhs.i * rhs.i) : Value::double_value(lhs.to_double() * rhs.to_double());
    if (op == "/") {
        const double denom = rhs.to_double();
        if (denom == 0.0) throw std::runtime_error("Division by zero");
        if (both_int && rhs.i != 0 && lhs.i % rhs.i == 0) return Value::int_value(lhs.i / rhs.i);
        return Value::double_value(lhs.to_double() / denom);
    }
    if (op == "%") {
        const long long denom = rhs.to_int();
        if (denom == 0) throw std::runtime_error("Modulo by zero");
        return Value::int_value(lhs.to_int() % denom);
    }

    throw std::runtime_error("unsupported operator '" + op + "'");
}

std::string InterpreterPHP::interpolate_string(const std::string& raw) {
    std::string out;
    size_t i = 0;
    while (i < raw.size()) {
        const char c = raw[i];
        if (c == '$' && i + 1 < raw.size() &&
            (std::isalpha(static_cast<unsigned char>(raw[i + 1])) != 0 || raw[i + 1] == '_')) {
            size_t j = i + 1;
            std::string name;
            while (j < raw.size() && (std::isalnum(static_cast<unsigned char>(raw[j])) != 0 || raw[j] == '_')) {
                name.push_back(raw[j++]);
            }
            Value val = get_variable(name);

            if (j < raw.size() && raw[j] == '[') {
                size_t k = j + 1;
                std::string key;
                while (k < raw.size() && raw[k] != ']') key.push_back(raw[k++]);
                if (k < raw.size()) {
                    const bool numeric = !key.empty() &&
                        (std::isdigit(static_cast<unsigned char>(key[0])) != 0 || key[0] == '-');
                    Value key_val = numeric ? Value::int_value(std::stoll(key)) : Value::string_value(key);
                    if (val.is_array()) {
                        Value* found = val.find(key_val);
                        out += found ? found->to_string() : "";
                    }
                    i = k + 1;
                    continue;
                }
            }

            out += val.to_string();
            i = j;
            continue;
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

Value InterpreterPHP::eval(const NodePtr& node) {
    switch (node->type) {
        case NodeType::IntLiteral: return Value::int_value(node->int_value);
        case NodeType::DoubleLiteral: return Value::double_value(node->double_value);
        case NodeType::BoolLiteral: return Value::bool_value(node->bool_value);
        case NodeType::NullLiteral: return Value::null_value();
        case NodeType::StringLiteral:
            return Value::string_value(node->interpolate ? interpolate_string(node->str_value) : node->str_value);
        case NodeType::Variable: return get_variable(node->str_value);
        case NodeType::Identifier: return Value::string_value(node->str_value);

        case NodeType::ArrayLiteral: {
            Value out = Value::array_value();
            for (const auto& child : node->children) {
                if (child->type == NodeType::KeyValue) {
                    out.set(eval(child->children[0]), clone(eval(child->children[1])));
                } else {
                    out.push(clone(eval(child)));
                }
            }
            return out;
        }

        case NodeType::ArrayAccess: {
            Value base = eval(node->children[0]);
            if (!base.is_array()) return Value::null_value();
            Value key = eval(node->children[1]);
            Value* found = base.find(key);
            return found ? clone(*found) : Value::null_value();
        }

        case NodeType::Assign: {
            Value value = clone(eval(node->children[1]));
            Value* ptr = lvalue_ref(node->children[0], true);
            *ptr = value;
            return value;
        }

        case NodeType::CompoundAssign: {
            Value* ptr = lvalue_ref(node->children[0], true);
            Value new_val = eval_binary(node->str_value, *ptr, eval(node->children[1]));
            *ptr = new_val;
            return new_val;
        }

        case NodeType::BinaryOp:
            return eval_binary(node->str_value, eval(node->children[0]), eval(node->children[1]));

        case NodeType::UnaryNot:
            return Value::bool_value(!eval(node->children[0]).to_bool());

        case NodeType::UnaryMinus: {
            Value v = eval(node->children[0]);
            if (v.type == ValueType::Double) return Value::double_value(-v.d);
            return Value::int_value(-v.to_int());
        }

        case NodeType::LogicalAnd: {
            if (!eval(node->children[0]).to_bool()) return Value::bool_value(false);
            return Value::bool_value(eval(node->children[1]).to_bool());
        }

        case NodeType::LogicalOr: {
            if (eval(node->children[0]).to_bool()) return Value::bool_value(true);
            return Value::bool_value(eval(node->children[1]).to_bool());
        }

        case NodeType::Ternary: {
            Value cond = eval(node->children[0]);
            if (node->bool_value) { // shorthand a ?: b
                return cond.to_bool() ? cond : eval(node->children[2]);
            }
            return cond.to_bool() ? eval(node->children[1]) : eval(node->children[2]);
        }

        case NodeType::FunctionCall: {
            if (node->str_value == "isset") {
                bool all_set = true;
                for (const auto& arg : node->children) {
                    if (arg->type == NodeType::Variable) {
                        Value v = get_variable(arg->str_value);
                        if (v.type == ValueType::Null) { all_set = false; break; }
                    } else if (eval(arg).type == ValueType::Null) {
                        all_set = false;
                        break;
                    }
                }
                return Value::bool_value(all_set);
            }
            if (node->str_value == "empty" && node->children.size() == 1) {
                return Value::bool_value(!eval(node->children[0]).to_bool());
            }
            if (node->str_value == "array_push" && !node->children.empty()) {
                Value* arr_ptr = lvalue_ref(node->children[0], true);
                if (arr_ptr->type != ValueType::Array) *arr_ptr = Value::array_value();
                for (size_t k = 1; k < node->children.size(); ++k) {
                    arr_ptr->push(clone(eval(node->children[k])));
                }
                return Value::int_value(static_cast<long long>(arr_ptr->arr->size()));
            }

            std::vector<Value> args;
            args.reserve(node->children.size());
            for (const auto& child : node->children) args.push_back(clone(eval(child)));
            return call_function(node->str_value, args);
        }

        default:
            throw std::runtime_error("cannot evaluate this construct as an expression");
    }
}

bool InterpreterPHP::call_builtin(const std::string& name, std::vector<Value>& args, Value& result) {
    auto arg_or_null = [&](size_t idx) { return idx < args.size() ? args[idx] : Value::null_value(); };

    if (name == "strlen") { result = Value::int_value(static_cast<long long>(arg_or_null(0).to_string().size())); return true; }
    if (name == "count" || name == "sizeof") {
        const Value v = arg_or_null(0);
        result = Value::int_value(v.is_array() && v.arr ? static_cast<long long>(v.arr->size()) : 0);
        return true;
    }
    if (name == "strtoupper") {
        std::string s = arg_or_null(0).to_string();
        for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        result = Value::string_value(s);
        return true;
    }
    if (name == "strtolower") {
        std::string s = arg_or_null(0).to_string();
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        result = Value::string_value(s);
        return true;
    }
    if (name == "trim") {
        std::string s = arg_or_null(0).to_string();
        const size_t b = s.find_first_not_of(" \t\n\r");
        const size_t e = s.find_last_not_of(" \t\n\r");
        result = Value::string_value(b == std::string::npos ? "" : s.substr(b, e - b + 1));
        return true;
    }
    if (name == "str_repeat") {
        const std::string s = arg_or_null(0).to_string();
        const long long times = arg_or_null(1).to_int();
        std::string out;
        for (long long k = 0; k < times; ++k) out += s;
        result = Value::string_value(out);
        return true;
    }
    if (name == "implode" || name == "join") {
        std::string glue;
        Value arr_val;
        if (args.size() >= 2) { glue = args[0].to_string(); arr_val = args[1]; }
        else if (args.size() == 1) { arr_val = args[0]; }
        std::string out;
        if (arr_val.is_array() && arr_val.arr) {
            bool first = true;
            for (const auto& kv : *arr_val.arr) {
                if (!first) out += glue;
                out += kv.second.to_string();
                first = false;
            }
        }
        result = Value::string_value(out);
        return true;
    }
    if (name == "explode") {
        const std::string sep = arg_or_null(0).to_string();
        const std::string str = arg_or_null(1).to_string();
        Value out = Value::array_value();
        if (sep.empty()) {
            out.push(Value::string_value(str));
        } else {
            size_t pos = 0;
            while (true) {
                const size_t next = str.find(sep, pos);
                if (next == std::string::npos) { out.push(Value::string_value(str.substr(pos))); break; }
                out.push(Value::string_value(str.substr(pos, next - pos)));
                pos = next + sep.size();
            }
        }
        result = out;
        return true;
    }
    if (name == "is_array") { result = Value::bool_value(arg_or_null(0).is_array()); return true; }
    if (name == "is_string") { result = Value::bool_value(arg_or_null(0).type == ValueType::String); return true; }
    if (name == "is_numeric") {
        const Value v = arg_or_null(0);
        result = Value::bool_value(v.type == ValueType::Int || v.type == ValueType::Double);
        return true;
    }
    if (name == "intval") { result = Value::int_value(arg_or_null(0).to_int()); return true; }
    if (name == "floatval" || name == "doubleval") { result = Value::double_value(arg_or_null(0).to_double()); return true; }
    if (name == "strval") { result = Value::string_value(arg_or_null(0).to_string()); return true; }
    if (name == "boolval") { result = Value::bool_value(arg_or_null(0).to_bool()); return true; }
    if (name == "htmlspecialchars") {
        const std::string s = arg_or_null(0).to_string();
        std::string out;
        for (const char c : s) {
            switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                case '\'': out += "&#039;"; break;
                default: out.push_back(c); break;
            }
        }
        result = Value::string_value(out);
        return true;
    }
    if (name == "print_r") { result = Value::string_value(print_r_string(arg_or_null(0), 0)); return true; }
    if (name == "in_array") {
        const Value needle = arg_or_null(0);
        const Value haystack = arg_or_null(1);
        bool found = false;
        if (haystack.is_array() && haystack.arr) {
            for (const auto& kv : *haystack.arr) {
                if (kv.second.loose_equals(needle)) { found = true; break; }
            }
        }
        result = Value::bool_value(found);
        return true;
    }
    if (name == "array_keys") {
        Value out = Value::array_value();
        const Value v = arg_or_null(0);
        if (v.is_array() && v.arr) for (const auto& kv : *v.arr) out.push(kv.first);
        result = out;
        return true;
    }
    if (name == "array_values") {
        Value out = Value::array_value();
        const Value v = arg_or_null(0);
        if (v.is_array() && v.arr) for (const auto& kv : *v.arr) out.push(kv.second);
        result = out;
        return true;
    }
    if (name == "max" || name == "min") {
        std::vector<Value> pool;
        if (args.size() == 1 && args[0].is_array() && args[0].arr) {
            for (const auto& kv : *args[0].arr) pool.push_back(kv.second);
        } else {
            pool = args;
        }
        if (pool.empty()) { result = Value::null_value(); return true; }
        Value best = pool[0];
        for (const auto& v : pool) {
            const bool better = (name == "max") ? (v.to_double() > best.to_double()) : (v.to_double() < best.to_double());
            if (better) best = v;
        }
        result = best;
        return true;
    }

    return false;
}

Value InterpreterPHP::call_function(const std::string& name, std::vector<Value>& args) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        Value result;
        if (call_builtin(name, args, result)) return result;
        throw std::runtime_error("Call to undefined function " + name + "()");
    }

    const NodePtr decl = it->second;
    const NodePtr& params = decl->children[0];
    const NodePtr& body = decl->children[1];

    Scope new_scope;
    for (size_t k = 0; k < params->children.size(); ++k) {
        const NodePtr& param = params->children[k];
        Value value;
        if (k < args.size()) value = clone(args[k]);
        else if (!param->children.empty()) value = eval(param->children[0]);
        else value = Value::null_value();
        new_scope[param->str_value] = value;
    }

    scopes_.push_back(std::move(new_scope));
    scope_globals_.emplace_back();

    Flow flow;
    exec_block(body, flow);
    Value result = (flow.kind == FlowKind::Return) ? flow.return_value : Value::null_value();

    scope_globals_.pop_back();
    scopes_.pop_back();
    return result;
}

std::string InterpreterPHP::run(const NodePtr& program) {
    output_.clear();
    scopes_.clear();
    scope_globals_.clear();
    functions_.clear();

    for (const auto& child : program->children) {
        if (child->type == NodeType::FunctionDecl) functions_[child->str_value] = child;
    }

    Flow flow;
    for (const auto& child : program->children) {
        if (child->type == NodeType::FunctionDecl) continue;
        exec_statement(child, flow);
        if (flow.kind != FlowKind::None) break;
    }
    return output_;
}

} // namespace plphp
