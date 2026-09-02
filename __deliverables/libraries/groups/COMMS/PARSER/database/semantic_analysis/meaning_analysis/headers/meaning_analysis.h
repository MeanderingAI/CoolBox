/**
 * @file meaning_analysis.h
 * @brief SQL semantic / meaning analysis — validates AST nodes against a
 *        schema catalog to verify table existence, column resolution,
 *        type compatibility, aggregate correctness, and scope rules.
 *
 * Works on AST nodes produced by the syntax_analysis/ast library.
 * Requires a SchemaCatalog describing known tables and their columns.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <functional>

#include "../../../syntax_analysis/ast/headers/sql_ast.h"

namespace ml {
namespace sql {
namespace semantic {

using namespace ml::sql::ast;

// -----------------------------------------------------------------------
// Schema catalog types — describes the database schema for validation
// -----------------------------------------------------------------------

enum class DataType {
    INTEGER,
    REAL,
    TEXT,
    BLOB,
    BOOLEAN,
    DATE,
    DATETIME,
    TIMESTAMP,
    UNKNOWN,
};

const char* data_type_name(DataType dt) noexcept;

struct CatalogColumn {
    std::string name;
    DataType    type      = DataType::UNKNOWN;
    bool        nullable  = true;
    bool        is_pk     = false;
    bool        is_unique = false;
};

struct CatalogTable {
    std::string                table_name;
    std::vector<CatalogColumn> columns;

    /// Check if a column exists (case-insensitive).
    bool has_column(const std::string& col) const;

    /// Get column info (nullptr if not found).
    const CatalogColumn* find_column(const std::string& col) const;
};

/// In-memory schema catalog.
class SchemaCatalog {
public:
    /// Add a table definition.
    void add_table(CatalogTable table);

    /// Remove a table.
    void remove_table(const std::string& name);

    /// Check if a table exists.
    bool has_table(const std::string& name) const;

    /// Get table info (nullptr if not found).
    const CatalogTable* find_table(const std::string& name) const;

    /// List all table names.
    std::vector<std::string> table_names() const;

    /// Build catalog from CREATE TABLE statements.
    static SchemaCatalog from_statements(const std::vector<Statement>& stmts);

    /// Build catalog from SQL string containing CREATE TABLE statements.
    static SchemaCatalog from_sql(const std::string& ddl);

private:
    std::unordered_map<std::string, CatalogTable> tables_;

    static std::string normalize(const std::string& s);
};

// -----------------------------------------------------------------------
// Semantic diagnostic
// -----------------------------------------------------------------------
enum class SemanticSeverity {
    ERROR,
    WARNING,
    INFO,
};

const char* semantic_severity_name(SemanticSeverity s) noexcept;

struct SemanticDiagnostic {
    SemanticSeverity severity = SemanticSeverity::ERROR;
    std::string      rule;       // e.g. "TABLE_NOT_FOUND"
    std::string      message;
    std::string      context;    // optional extra info

    bool is_error()   const noexcept { return severity == SemanticSeverity::ERROR; }
    bool is_warning() const noexcept { return severity == SemanticSeverity::WARNING; }
};

struct SemanticResult {
    bool valid = true;
    std::vector<SemanticDiagnostic> diagnostics;

    size_t error_count() const;
    size_t warning_count() const;
    size_t info_count() const;

    void merge(const SemanticResult& other);
};

// -----------------------------------------------------------------------
// Semantic rule
// -----------------------------------------------------------------------
using SemanticRuleFunction = std::function<void(
    const Statement&, const SchemaCatalog&, SemanticResult&)>;

struct SemanticRule {
    std::string          id;
    std::string          description;
    SemanticSeverity     default_severity = SemanticSeverity::ERROR;
    SemanticRuleFunction check;
    bool                 enabled = true;
};

// -----------------------------------------------------------------------
// MeaningAnalyzer — the main semantic checker
// -----------------------------------------------------------------------
class MeaningAnalyzer {
public:
    MeaningAnalyzer();

    /// Set the schema catalog for validation.
    void set_catalog(const SchemaCatalog& catalog);
    const SchemaCatalog& catalog() const noexcept { return catalog_; }

    /// Validate a single statement.
    SemanticResult analyze(const Statement& stmt) const;

    /// Validate multiple statements.
    SemanticResult analyze(const std::vector<Statement>& stmts) const;

    /// Convenience: parse SQL and analyze.
    SemanticResult analyze_sql(const std::string& sql) const;

    /// Access rules.
    const std::vector<SemanticRule>& rules() const noexcept { return rules_; }

    /// Add a custom rule.
    void add_rule(SemanticRule rule);

    /// Enable / disable a rule by id.
    void set_rule_enabled(const std::string& id, bool enabled);

private:
    SchemaCatalog catalog_;
    std::vector<SemanticRule> rules_;

    void init_builtin_rules();

    // ---- Built-in semantic rules --------------------------------------

    // Table existence
    static void rule_table_not_found(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Column existence (resolves qualified and unqualified names)
    static void rule_column_not_found(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Duplicate column in INSERT
    static void rule_duplicate_insert_column(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Non-nullable column missing in INSERT
    static void rule_missing_not_null_column(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Type mismatch in comparisons (basic)
    static void rule_type_mismatch_comparison(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Aggregate in WHERE clause
    static void rule_aggregate_in_where(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Non-aggregated column in SELECT with GROUP BY
    static void rule_non_aggregated_column(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // UPDATE / DELETE on non-existent table
    static void rule_modify_nonexistent_table(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // DROP non-existent table (without IF EXISTS)
    static void rule_drop_nonexistent_table(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // CREATE table that already exists (without IF NOT EXISTS)
    static void rule_create_existing_table(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // Duplicate column names in CREATE TABLE
    static void rule_duplicate_column_def(
        const Statement& s, const SchemaCatalog& cat, SemanticResult& r);

    // ---- Expression walking helpers -----------------------------------
    static void collect_column_refs(const ExprPtr& expr,
                                    std::vector<std::string>& refs);
    static bool expr_has_aggregate(const ExprPtr& expr);
    static bool is_aggregate_function(const std::string& name);
};

} // namespace semantic
} // namespace sql
} // namespace ml
