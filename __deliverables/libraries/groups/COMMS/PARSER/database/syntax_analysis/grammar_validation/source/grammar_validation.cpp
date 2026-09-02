/**
 * @file grammar_validation.cpp
 * @brief SQL grammar validation implementation.
 */

#include "../headers/grammar_validation.h"

#include <algorithm>

namespace ml {
namespace sql {
namespace validation {

// -----------------------------------------------------------------------
// severity_name
// -----------------------------------------------------------------------
const char* severity_name(Severity s) noexcept {
    switch (s) {
        case Severity::ERROR:   return "ERROR";
        case Severity::WARNING: return "WARNING";
        case Severity::INFO:    return "INFO";
    }
    return "UNKNOWN";
}

// -----------------------------------------------------------------------
// ValidationResult
// -----------------------------------------------------------------------
size_t ValidationResult::error_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.is_error()) ++n;
    return n;
}

size_t ValidationResult::warning_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.is_warning()) ++n;
    return n;
}

size_t ValidationResult::info_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.severity == Severity::INFO) ++n;
    return n;
}

void ValidationResult::merge(const ValidationResult& other) {
    for (auto& d : other.diagnostics) {
        diagnostics.push_back(d);
    }
    if (!other.valid) valid = false;
}

// -----------------------------------------------------------------------
// Helper: add a diagnostic
// -----------------------------------------------------------------------
static void emit(ValidationResult& r, Severity sev, const std::string& rule,
                 const std::string& msg, const std::string& ctx = "") {
    Diagnostic d;
    d.severity = sev;
    d.rule     = rule;
    d.message  = msg;
    d.context  = ctx;
    r.diagnostics.push_back(std::move(d));
    if (sev == Severity::ERROR) r.valid = false;
}

// -----------------------------------------------------------------------
// GrammarValidator
// -----------------------------------------------------------------------
GrammarValidator::GrammarValidator() {
    init_builtin_rules();
}

void GrammarValidator::init_builtin_rules() {
    auto add = [&](const std::string& id, const std::string& desc,
                   Severity sev, RuleFunction fn) {
        Rule r;
        r.id = id;
        r.description = desc;
        r.default_severity = sev;
        r.check = std::move(fn);
        r.enabled = true;
        rules_.push_back(std::move(r));
    };

    // SELECT rules
    add("SELECT_NO_COLUMNS",
        "SELECT statement has no result columns",
        Severity::ERROR, rule_select_no_columns);

    add("SELECT_NO_FROM",
        "SELECT without FROM clause (not necessarily an error for literals)",
        Severity::WARNING, rule_select_no_from);

    add("SELECT_STAR_WITH_GROUP_BY",
        "SELECT * combined with GROUP BY is usually incorrect",
        Severity::WARNING, rule_select_star_with_group_by);

    // INSERT rules
    add("INSERT_NO_TABLE",
        "INSERT statement has no target table",
        Severity::ERROR, rule_insert_no_table);

    add("INSERT_NO_VALUES",
        "INSERT statement has no value rows",
        Severity::ERROR, rule_insert_no_values);

    add("INSERT_COLUMN_VALUE_MISMATCH",
        "INSERT column count does not match value count",
        Severity::ERROR, rule_insert_column_value_mismatch);

    // UPDATE rules
    add("UPDATE_NO_TABLE",
        "UPDATE statement has no target table",
        Severity::ERROR, rule_update_no_table);

    add("UPDATE_NO_SET",
        "UPDATE statement has no SET assignments",
        Severity::ERROR, rule_update_no_set);

    add("UPDATE_NO_WHERE",
        "UPDATE without WHERE clause affects all rows",
        Severity::WARNING, rule_update_no_where);

    // DELETE rules
    add("DELETE_NO_TABLE",
        "DELETE statement has no target table",
        Severity::ERROR, rule_delete_no_table);

    add("DELETE_NO_WHERE",
        "DELETE without WHERE clause deletes all rows",
        Severity::WARNING, rule_delete_no_where);

    // CREATE TABLE rules
    add("CREATE_NO_TABLE",
        "CREATE TABLE has no table name",
        Severity::ERROR, rule_create_no_table);

    add("CREATE_NO_COLUMNS",
        "CREATE TABLE has no column definitions",
        Severity::ERROR, rule_create_no_columns);

    add("CREATE_COLUMN_NO_TYPE",
        "Column definition is missing a type",
        Severity::ERROR, rule_create_column_no_type);

    // DROP TABLE rules
    add("DROP_NO_TABLE",
        "DROP TABLE has no table name",
        Severity::ERROR, rule_drop_no_table);

    // JOIN rules
    add("JOIN_NO_ON",
        "JOIN clause has no ON condition (except CROSS/NATURAL)",
        Severity::WARNING, rule_join_no_on);

    // Misc rules
    add("AMBIGUOUS_COLUMN",
        "Unqualified column in multi-table query may be ambiguous",
        Severity::INFO, rule_ambiguous_column);
}

ValidationResult GrammarValidator::validate(const Statement& stmt) const {
    ValidationResult result;
    for (auto& rule : rules_) {
        if (rule.enabled) {
            rule.check(stmt, result);
        }
    }
    return result;
}

ValidationResult GrammarValidator::validate(const std::vector<Statement>& stmts) const {
    ValidationResult combined;
    for (auto& stmt : stmts) {
        combined.merge(validate(stmt));
    }
    return combined;
}

ValidationResult GrammarValidator::validate_sql(const std::string& sql) const {
    SQLAstGenerator gen;
    try {
        auto stmts = gen.parse(sql);
        return validate(stmts);
    } catch (const std::exception& ex) {
        ValidationResult r;
        emit(r, Severity::ERROR, "PARSE_ERROR", ex.what());
        return r;
    }
}

void GrammarValidator::add_rule(Rule rule) {
    rules_.push_back(std::move(rule));
}

void GrammarValidator::set_rule_enabled(const std::string& id, bool enabled) {
    for (auto& r : rules_) {
        if (r.id == id) { r.enabled = enabled; return; }
    }
}

void GrammarValidator::errors_only() {
    for (auto& r : rules_) {
        if (r.default_severity != Severity::ERROR) {
            r.enabled = false;
        }
    }
}

// -----------------------------------------------------------------------
// Built-in rule implementations
// -----------------------------------------------------------------------

void GrammarValidator::rule_select_no_columns(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    if (sel.columns.empty()) {
        emit(r, Severity::ERROR, "SELECT_NO_COLUMNS",
             "SELECT statement has no result columns");
    }
}

void GrammarValidator::rule_select_no_from(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    if (sel.from_table.empty()) {
        // Only warn if there are non-literal columns (SELECT 1 is fine)
        bool all_literals = true;
        for (auto& col : sel.columns) {
            if (col->type != ExprType::LITERAL) { all_literals = false; break; }
        }
        if (!all_literals) {
            emit(r, Severity::WARNING, "SELECT_NO_FROM",
                 "SELECT references columns but has no FROM clause");
        }
    }
}

void GrammarValidator::rule_select_star_with_group_by(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    if (sel.group_by.empty()) return;
    for (auto& col : sel.columns) {
        if (col->type == ExprType::STAR) {
            emit(r, Severity::WARNING, "SELECT_STAR_WITH_GROUP_BY",
                 "SELECT * combined with GROUP BY is usually incorrect",
                 "Consider listing specific columns");
        }
    }
}

void GrammarValidator::rule_insert_no_table(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::INSERT) return;
    auto& ins = std::get<InsertStmt>(s.node);
    if (ins.table.empty()) {
        emit(r, Severity::ERROR, "INSERT_NO_TABLE",
             "INSERT statement has no target table");
    }
}

void GrammarValidator::rule_insert_no_values(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::INSERT) return;
    auto& ins = std::get<InsertStmt>(s.node);
    if (ins.value_rows.empty()) {
        emit(r, Severity::ERROR, "INSERT_NO_VALUES",
             "INSERT statement has no value rows");
    }
}

void GrammarValidator::rule_insert_column_value_mismatch(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::INSERT) return;
    auto& ins = std::get<InsertStmt>(s.node);
    if (ins.columns.empty()) return; // no explicit column list — skip
    for (size_t i = 0; i < ins.value_rows.size(); ++i) {
        if (ins.value_rows[i].size() != ins.columns.size()) {
            emit(r, Severity::ERROR, "INSERT_COLUMN_VALUE_MISMATCH",
                 "Row " + std::to_string(i + 1) + ": expected "
                 + std::to_string(ins.columns.size()) + " values, got "
                 + std::to_string(ins.value_rows[i].size()));
        }
    }
}

void GrammarValidator::rule_update_no_table(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::UPDATE) return;
    auto& upd = std::get<UpdateStmt>(s.node);
    if (upd.table.empty()) {
        emit(r, Severity::ERROR, "UPDATE_NO_TABLE",
             "UPDATE statement has no target table");
    }
}

void GrammarValidator::rule_update_no_set(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::UPDATE) return;
    auto& upd = std::get<UpdateStmt>(s.node);
    if (upd.assignments.empty()) {
        emit(r, Severity::ERROR, "UPDATE_NO_SET",
             "UPDATE statement has no SET assignments");
    }
}

void GrammarValidator::rule_update_no_where(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::UPDATE) return;
    auto& upd = std::get<UpdateStmt>(s.node);
    if (!upd.where_clause) {
        emit(r, Severity::WARNING, "UPDATE_NO_WHERE",
             "UPDATE without WHERE clause will affect all rows");
    }
}

void GrammarValidator::rule_delete_no_table(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::DELETE_STMT) return;
    auto& del = std::get<DeleteStmt>(s.node);
    if (del.table.empty()) {
        emit(r, Severity::ERROR, "DELETE_NO_TABLE",
             "DELETE statement has no target table");
    }
}

void GrammarValidator::rule_delete_no_where(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::DELETE_STMT) return;
    auto& del = std::get<DeleteStmt>(s.node);
    if (!del.where_clause) {
        emit(r, Severity::WARNING, "DELETE_NO_WHERE",
             "DELETE without WHERE clause will delete all rows");
    }
}

void GrammarValidator::rule_create_no_table(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::CREATE_TABLE) return;
    auto& ct = std::get<CreateTableStmt>(s.node);
    if (ct.table.empty()) {
        emit(r, Severity::ERROR, "CREATE_NO_TABLE",
             "CREATE TABLE has no table name");
    }
}

void GrammarValidator::rule_create_no_columns(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::CREATE_TABLE) return;
    auto& ct = std::get<CreateTableStmt>(s.node);
    if (ct.columns.empty()) {
        emit(r, Severity::ERROR, "CREATE_NO_COLUMNS",
             "CREATE TABLE has no column definitions");
    }
}

void GrammarValidator::rule_create_column_no_type(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::CREATE_TABLE) return;
    auto& ct = std::get<CreateTableStmt>(s.node);
    for (auto& col : ct.columns) {
        if (col.type_name.empty()) {
            emit(r, Severity::ERROR, "CREATE_COLUMN_NO_TYPE",
                 "Column '" + col.name + "' is missing a data type");
        }
    }
}

void GrammarValidator::rule_drop_no_table(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::DROP_TABLE) return;
    auto& dt = std::get<DropTableStmt>(s.node);
    if (dt.table.empty()) {
        emit(r, Severity::ERROR, "DROP_NO_TABLE",
             "DROP TABLE has no table name");
    }
}

void GrammarValidator::rule_join_no_on(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    for (auto& j : sel.joins) {
        if (j.type != JoinType::CROSS && j.type != JoinType::NATURAL) {
            if (!j.on_condition) {
                emit(r, Severity::WARNING, "JOIN_NO_ON",
                     "JOIN on table '" + j.table + "' has no ON condition");
            }
        }
    }
}

void GrammarValidator::rule_order_by_in_subquery(const Statement& /*s*/, ValidationResult& /*r*/) {
    // Placeholder for future subquery support
}

void GrammarValidator::rule_ambiguous_column(const Statement& s, ValidationResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    // Only relevant when there are joins (multiple tables)
    if (sel.joins.empty()) return;

    for (auto& col : sel.columns) {
        if (col->type == ExprType::IDENTIFIER && col->value.find('.') == std::string::npos) {
            emit(r, Severity::INFO, "AMBIGUOUS_COLUMN",
                 "Column '" + col->value + "' is unqualified in a multi-table query",
                 "Consider using table.column notation");
        }
    }
}

} // namespace validation
} // namespace sql
} // namespace ml
