/**
 * @file grammar_validation.h
 * @brief SQL grammar validation — validates AST nodes against SQL grammar
 *        rules, producing structured diagnostic messages.
 *
 * Independent from the tokenizer and AST generator; takes AST nodes as input.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../../ast/headers/sql_ast.h"

namespace ml {
namespace sql {
namespace validation {

using namespace ml::sql::ast;

// -----------------------------------------------------------------------
// Diagnostic severity
// -----------------------------------------------------------------------
enum class Severity {
    ERROR,      // hard violation — query would fail
    WARNING,    // suspicious but technically allowed
    INFO,       // style / best-practice suggestion
};

const char* severity_name(Severity s) noexcept;

// -----------------------------------------------------------------------
// Diagnostic message
// -----------------------------------------------------------------------
struct Diagnostic {
    Severity    severity = Severity::ERROR;
    std::string rule;       // rule identifier, e.g. "SELECT_NO_COLUMNS"
    std::string message;    // human-readable explanation
    std::string context;    // optional SQL fragment / node description

    bool is_error()   const noexcept { return severity == Severity::ERROR; }
    bool is_warning() const noexcept { return severity == Severity::WARNING; }
};

// -----------------------------------------------------------------------
// Validation result
// -----------------------------------------------------------------------
struct ValidationResult {
    bool valid = true;                     // no ERRORs found
    std::vector<Diagnostic> diagnostics;

    /// Convenience: count by severity.
    size_t error_count() const;
    size_t warning_count() const;
    size_t info_count() const;

    /// Append another result.
    void merge(const ValidationResult& other);
};

// -----------------------------------------------------------------------
// Validation rule — a single pluggable check
// -----------------------------------------------------------------------
using RuleFunction = std::function<void(const Statement&, ValidationResult&)>;

struct Rule {
    std::string  id;            // e.g. "NO_TABLE_IN_SELECT"
    std::string  description;
    Severity     default_severity = Severity::ERROR;
    RuleFunction check;
    bool         enabled = true;
};

// -----------------------------------------------------------------------
// GrammarValidator — collects rules and validates statements
// -----------------------------------------------------------------------
class GrammarValidator {
public:
    GrammarValidator();

    /// Validate a single statement.
    ValidationResult validate(const Statement& stmt) const;

    /// Validate multiple statements.
    ValidationResult validate(const std::vector<Statement>& stmts) const;

    /// Convenience: parse SQL string and validate.
    ValidationResult validate_sql(const std::string& sql) const;

    /// Access rules.
    const std::vector<Rule>& rules() const noexcept { return rules_; }

    /// Add a custom rule.
    void add_rule(Rule rule);

    /// Enable / disable a rule by id.
    void set_rule_enabled(const std::string& id, bool enabled);

    /// Disable all warnings (keep only errors).
    void errors_only();

private:
    std::vector<Rule> rules_;

    /// Register built-in rules.
    void init_builtin_rules();

    // ---- Built-in rule implementations --------------------------------
    static void rule_select_no_columns(const Statement& s, ValidationResult& r);
    static void rule_select_no_from(const Statement& s, ValidationResult& r);
    static void rule_insert_no_table(const Statement& s, ValidationResult& r);
    static void rule_insert_no_values(const Statement& s, ValidationResult& r);
    static void rule_insert_column_value_mismatch(const Statement& s, ValidationResult& r);
    static void rule_update_no_table(const Statement& s, ValidationResult& r);
    static void rule_update_no_set(const Statement& s, ValidationResult& r);
    static void rule_update_no_where(const Statement& s, ValidationResult& r);
    static void rule_delete_no_table(const Statement& s, ValidationResult& r);
    static void rule_delete_no_where(const Statement& s, ValidationResult& r);
    static void rule_create_no_table(const Statement& s, ValidationResult& r);
    static void rule_create_no_columns(const Statement& s, ValidationResult& r);
    static void rule_create_column_no_type(const Statement& s, ValidationResult& r);
    static void rule_drop_no_table(const Statement& s, ValidationResult& r);
    static void rule_join_no_on(const Statement& s, ValidationResult& r);
    static void rule_order_by_in_subquery(const Statement& s, ValidationResult& r);
    static void rule_select_star_with_group_by(const Statement& s, ValidationResult& r);
    static void rule_ambiguous_column(const Statement& s, ValidationResult& r);
};

} // namespace validation
} // namespace sql
} // namespace ml
