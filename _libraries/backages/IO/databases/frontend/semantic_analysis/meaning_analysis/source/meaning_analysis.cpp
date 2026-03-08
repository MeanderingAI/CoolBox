/**
 * @file meaning_analysis.cpp
 * @brief SQL semantic / meaning analysis implementation.
 */

#include "../headers/meaning_analysis.h"

#include <algorithm>
#include <cctype>

namespace ml {
namespace sql {
namespace semantic {

// -----------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------
static std::string to_upper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return out;
}

static void emit(SemanticResult& r, SemanticSeverity sev,
                 const std::string& rule, const std::string& msg,
                 const std::string& ctx = "") {
    SemanticDiagnostic d;
    d.severity = sev;
    d.rule     = rule;
    d.message  = msg;
    d.context  = ctx;
    r.diagnostics.push_back(std::move(d));
    if (sev == SemanticSeverity::ERROR) r.valid = false;
}

// -----------------------------------------------------------------------
// DataType name
// -----------------------------------------------------------------------
const char* data_type_name(DataType dt) noexcept {
    switch (dt) {
        case DataType::INTEGER:   return "INTEGER";
        case DataType::REAL:      return "REAL";
        case DataType::TEXT:      return "TEXT";
        case DataType::BLOB:      return "BLOB";
        case DataType::BOOLEAN:   return "BOOLEAN";
        case DataType::DATE:      return "DATE";
        case DataType::DATETIME:  return "DATETIME";
        case DataType::TIMESTAMP: return "TIMESTAMP";
        case DataType::UNKNOWN:   return "UNKNOWN";
    }
    return "UNKNOWN";
}

const char* semantic_severity_name(SemanticSeverity s) noexcept {
    switch (s) {
        case SemanticSeverity::ERROR:   return "ERROR";
        case SemanticSeverity::WARNING: return "WARNING";
        case SemanticSeverity::INFO:    return "INFO";
    }
    return "UNKNOWN";
}

// -----------------------------------------------------------------------
// SemanticResult
// -----------------------------------------------------------------------
size_t SemanticResult::error_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.is_error()) ++n;
    return n;
}

size_t SemanticResult::warning_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.is_warning()) ++n;
    return n;
}

size_t SemanticResult::info_count() const {
    size_t n = 0;
    for (auto& d : diagnostics) if (d.severity == SemanticSeverity::INFO) ++n;
    return n;
}

void SemanticResult::merge(const SemanticResult& other) {
    for (auto& d : other.diagnostics) diagnostics.push_back(d);
    if (!other.valid) valid = false;
}

// -----------------------------------------------------------------------
// CatalogTable
// -----------------------------------------------------------------------
bool CatalogTable::has_column(const std::string& col) const {
    std::string upper_col = to_upper(col);
    for (auto& c : columns) {
        if (to_upper(c.name) == upper_col) return true;
    }
    return false;
}

const CatalogColumn* CatalogTable::find_column(const std::string& col) const {
    std::string upper_col = to_upper(col);
    for (auto& c : columns) {
        if (to_upper(c.name) == upper_col) return &c;
    }
    return nullptr;
}

// -----------------------------------------------------------------------
// SchemaCatalog
// -----------------------------------------------------------------------
std::string SchemaCatalog::normalize(const std::string& s) {
    return to_upper(s);
}

void SchemaCatalog::add_table(CatalogTable table) {
    std::string key = normalize(table.table_name);
    tables_[key] = std::move(table);
}

void SchemaCatalog::remove_table(const std::string& name) {
    tables_.erase(normalize(name));
}

bool SchemaCatalog::has_table(const std::string& name) const {
    return tables_.count(normalize(name)) > 0;
}

const CatalogTable* SchemaCatalog::find_table(const std::string& name) const {
    auto it = tables_.find(normalize(name));
    if (it != tables_.end()) return &it->second;
    return nullptr;
}

std::vector<std::string> SchemaCatalog::table_names() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (auto& [k, v] : tables_) names.push_back(v.table_name);
    return names;
}

static DataType sql_type_to_data_type(const std::string& type_name) {
    std::string upper = to_upper(type_name);
    // Strip parenthesized size: VARCHAR(255) -> VARCHAR
    auto paren = upper.find('(');
    if (paren != std::string::npos) upper = upper.substr(0, paren);

    if (upper == "INTEGER" || upper == "INT" || upper == "BIGINT" ||
        upper == "SMALLINT" || upper == "TINYINT" || upper == "SERIAL" ||
        upper == "BIGSERIAL")
        return DataType::INTEGER;
    if (upper == "REAL" || upper == "FLOAT" || upper == "DOUBLE" ||
        upper == "DECIMAL" || upper == "NUMERIC")
        return DataType::REAL;
    if (upper == "TEXT" || upper == "VARCHAR" || upper == "CHAR")
        return DataType::TEXT;
    if (upper == "BLOB")
        return DataType::BLOB;
    if (upper == "BOOLEAN" || upper == "BOOL")
        return DataType::BOOLEAN;
    if (upper == "DATE")
        return DataType::DATE;
    if (upper == "DATETIME")
        return DataType::DATETIME;
    if (upper == "TIMESTAMP")
        return DataType::TIMESTAMP;
    return DataType::UNKNOWN;
}

SchemaCatalog SchemaCatalog::from_statements(const std::vector<Statement>& stmts) {
    SchemaCatalog cat;
    for (auto& stmt : stmts) {
        if (stmt.type != StmtType::CREATE_TABLE) continue;
        auto& ct = std::get<CreateTableStmt>(stmt.node);

        CatalogTable table;
        table.table_name = ct.table;
        for (auto& col_def : ct.columns) {
            CatalogColumn cc;
            cc.name      = col_def.name;
            cc.type      = sql_type_to_data_type(col_def.type_name);
            cc.nullable  = !col_def.not_null && !col_def.primary_key;
            cc.is_pk     = col_def.primary_key;
            cc.is_unique = col_def.unique || col_def.primary_key;
            table.columns.push_back(std::move(cc));
        }
        cat.add_table(std::move(table));
    }
    return cat;
}

SchemaCatalog SchemaCatalog::from_sql(const std::string& ddl) {
    SQLAstGenerator gen;
    auto stmts = gen.parse(ddl);
    return from_statements(stmts);
}

// -----------------------------------------------------------------------
// MeaningAnalyzer
// -----------------------------------------------------------------------
MeaningAnalyzer::MeaningAnalyzer() {
    init_builtin_rules();
}

void MeaningAnalyzer::init_builtin_rules() {
    auto add = [&](const std::string& id, const std::string& desc,
                   SemanticSeverity sev, SemanticRuleFunction fn) {
        SemanticRule r;
        r.id = id;
        r.description = desc;
        r.default_severity = sev;
        r.check = std::move(fn);
        r.enabled = true;
        rules_.push_back(std::move(r));
    };

    add("TABLE_NOT_FOUND",
        "Referenced table does not exist in the schema catalog",
        SemanticSeverity::ERROR, rule_table_not_found);

    add("COLUMN_NOT_FOUND",
        "Referenced column does not exist in the target table",
        SemanticSeverity::ERROR, rule_column_not_found);

    add("DUPLICATE_INSERT_COLUMN",
        "Duplicate column name in INSERT column list",
        SemanticSeverity::ERROR, rule_duplicate_insert_column);

    add("MISSING_NOT_NULL_COLUMN",
        "NOT NULL column is missing from INSERT and has no default",
        SemanticSeverity::WARNING, rule_missing_not_null_column);

    add("TYPE_MISMATCH_COMPARISON",
        "Comparing incompatible types",
        SemanticSeverity::WARNING, rule_type_mismatch_comparison);

    add("AGGREGATE_IN_WHERE",
        "Aggregate function used in WHERE clause (use HAVING instead)",
        SemanticSeverity::ERROR, rule_aggregate_in_where);

    add("NON_AGGREGATED_COLUMN",
        "Non-aggregated column in SELECT with GROUP BY",
        SemanticSeverity::WARNING, rule_non_aggregated_column);

    add("MODIFY_NONEXISTENT_TABLE",
        "UPDATE or DELETE on a table that doesn't exist",
        SemanticSeverity::ERROR, rule_modify_nonexistent_table);

    add("DROP_NONEXISTENT_TABLE",
        "DROP TABLE on a non-existent table without IF EXISTS",
        SemanticSeverity::ERROR, rule_drop_nonexistent_table);

    add("CREATE_EXISTING_TABLE",
        "CREATE TABLE on an already-existing table without IF NOT EXISTS",
        SemanticSeverity::ERROR, rule_create_existing_table);

    add("DUPLICATE_COLUMN_DEF",
        "Duplicate column name in CREATE TABLE",
        SemanticSeverity::ERROR, rule_duplicate_column_def);
}

void MeaningAnalyzer::set_catalog(const SchemaCatalog& catalog) {
    catalog_ = catalog;
}

SemanticResult MeaningAnalyzer::analyze(const Statement& stmt) const {
    SemanticResult result;
    for (auto& rule : rules_) {
        if (rule.enabled) {
            rule.check(stmt, catalog_, result);
        }
    }
    return result;
}

SemanticResult MeaningAnalyzer::analyze(const std::vector<Statement>& stmts) const {
    SemanticResult combined;
    for (auto& stmt : stmts) {
        combined.merge(analyze(stmt));
    }
    return combined;
}

SemanticResult MeaningAnalyzer::analyze_sql(const std::string& sql) const {
    SQLAstGenerator gen;
    try {
        auto stmts = gen.parse(sql);
        return analyze(stmts);
    } catch (const std::exception& ex) {
        SemanticResult r;
        emit(r, SemanticSeverity::ERROR, "PARSE_ERROR", ex.what());
        return r;
    }
}

void MeaningAnalyzer::add_rule(SemanticRule rule) {
    rules_.push_back(std::move(rule));
}

void MeaningAnalyzer::set_rule_enabled(const std::string& id, bool enabled) {
    for (auto& r : rules_) {
        if (r.id == id) { r.enabled = enabled; return; }
    }
}

// -----------------------------------------------------------------------
// Expression walking helpers
// -----------------------------------------------------------------------
void MeaningAnalyzer::collect_column_refs(const ExprPtr& expr,
                                          std::vector<std::string>& refs) {
    if (!expr) return;

    if (expr->type == ExprType::IDENTIFIER) {
        refs.push_back(expr->value);
        return;
    }

    // Recurse into children
    collect_column_refs(expr->left, refs);
    collect_column_refs(expr->right, refs);
    for (auto& arg : expr->args) {
        collect_column_refs(arg, refs);
    }
}

bool MeaningAnalyzer::is_aggregate_function(const std::string& name) {
    static const std::unordered_set<std::string> aggs = {
        "COUNT", "SUM", "AVG", "MIN", "MAX", "TOTAL", "GROUP_CONCAT"
    };
    return aggs.count(to_upper(name)) > 0;
}

bool MeaningAnalyzer::expr_has_aggregate(const ExprPtr& expr) {
    if (!expr) return false;

    if (expr->type == ExprType::FUNCTION_CALL) {
        if (is_aggregate_function(expr->value)) return true;
    }

    if (expr_has_aggregate(expr->left)) return true;
    if (expr_has_aggregate(expr->right)) return true;
    for (auto& arg : expr->args) {
        if (expr_has_aggregate(arg)) return true;
    }
    return false;
}

// -----------------------------------------------------------------------
// Built-in rule: TABLE_NOT_FOUND
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_table_not_found(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {

    if (s.type == StmtType::SELECT) {
        auto& sel = std::get<SelectStmt>(s.node);
        if (!sel.from_table.empty() && !cat.has_table(sel.from_table)) {
            emit(r, SemanticSeverity::ERROR, "TABLE_NOT_FOUND",
                 "Table '" + sel.from_table + "' does not exist");
        }
        for (auto& j : sel.joins) {
            if (!j.table.empty() && !cat.has_table(j.table)) {
                emit(r, SemanticSeverity::ERROR, "TABLE_NOT_FOUND",
                     "Joined table '" + j.table + "' does not exist");
            }
        }
    } else if (s.type == StmtType::INSERT) {
        auto& ins = std::get<InsertStmt>(s.node);
        if (!ins.table.empty() && !cat.has_table(ins.table)) {
            emit(r, SemanticSeverity::ERROR, "TABLE_NOT_FOUND",
                 "Table '" + ins.table + "' does not exist");
        }
    }
    // UPDATE and DELETE handled by rule_modify_nonexistent_table
}

// -----------------------------------------------------------------------
// Built-in rule: COLUMN_NOT_FOUND
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_column_not_found(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {

    auto check_columns = [&](const std::string& table_name,
                             const std::vector<std::string>& col_refs) {
        auto* table = cat.find_table(table_name);
        if (!table) return; // table not found is caught by another rule

        for (auto& ref : col_refs) {
            std::string col = ref;
            // Handle qualified name: table.column
            auto dot = ref.find('.');
            if (dot != std::string::npos) {
                std::string tbl = ref.substr(0, dot);
                col = ref.substr(dot + 1);
                // Check if qualified table matches
                if (to_upper(tbl) != to_upper(table_name)) {
                    // Might refer to a joined table — check that
                    auto* other = cat.find_table(tbl);
                    if (other && !other->has_column(col)) {
                        emit(r, SemanticSeverity::ERROR, "COLUMN_NOT_FOUND",
                             "Column '" + col + "' not found in table '" + tbl + "'");
                    }
                    continue;
                }
            }
            if (col != "*" && !table->has_column(col)) {
                emit(r, SemanticSeverity::ERROR, "COLUMN_NOT_FOUND",
                     "Column '" + col + "' not found in table '" + table_name + "'");
            }
        }
    };

    if (s.type == StmtType::SELECT) {
        auto& sel = std::get<SelectStmt>(s.node);
        if (sel.from_table.empty()) return;

        std::vector<std::string> refs;
        for (auto& col : sel.columns) collect_column_refs(col, refs);
        if (sel.where_clause) collect_column_refs(sel.where_clause, refs);
        for (auto& gb : sel.group_by) collect_column_refs(gb, refs);
        if (sel.having) collect_column_refs(sel.having, refs);
        for (auto& ob : sel.order_by) collect_column_refs(ob.expr, refs);

        check_columns(sel.from_table, refs);

    } else if (s.type == StmtType::INSERT) {
        auto& ins = std::get<InsertStmt>(s.node);
        if (ins.table.empty()) return;
        auto* table = cat.find_table(ins.table);
        if (!table) return;

        for (auto& col_name : ins.columns) {
            if (!table->has_column(col_name)) {
                emit(r, SemanticSeverity::ERROR, "COLUMN_NOT_FOUND",
                     "Column '" + col_name + "' not found in table '" + ins.table + "'");
            }
        }

    } else if (s.type == StmtType::UPDATE) {
        auto& upd = std::get<UpdateStmt>(s.node);
        if (upd.table.empty()) return;
        auto* table = cat.find_table(upd.table);
        if (!table) return;

        for (auto& [col_name, _] : upd.assignments) {
            if (!table->has_column(col_name)) {
                emit(r, SemanticSeverity::ERROR, "COLUMN_NOT_FOUND",
                     "Column '" + col_name + "' not found in table '" + upd.table + "'");
            }
        }

        if (upd.where_clause) {
            std::vector<std::string> refs;
            collect_column_refs(upd.where_clause, refs);
            check_columns(upd.table, refs);
        }
    }
}

// -----------------------------------------------------------------------
// Built-in rule: DUPLICATE_INSERT_COLUMN
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_duplicate_insert_column(
    const Statement& s, const SchemaCatalog& /*cat*/, SemanticResult& r) {
    if (s.type != StmtType::INSERT) return;
    auto& ins = std::get<InsertStmt>(s.node);

    std::unordered_set<std::string> seen;
    for (auto& col : ins.columns) {
        std::string upper = to_upper(col);
        if (seen.count(upper)) {
            emit(r, SemanticSeverity::ERROR, "DUPLICATE_INSERT_COLUMN",
                 "Duplicate column '" + col + "' in INSERT column list");
        }
        seen.insert(upper);
    }
}

// -----------------------------------------------------------------------
// Built-in rule: MISSING_NOT_NULL_COLUMN
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_missing_not_null_column(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {
    if (s.type != StmtType::INSERT) return;
    auto& ins = std::get<InsertStmt>(s.node);
    if (ins.columns.empty()) return; // no explicit columns — can't check

    auto* table = cat.find_table(ins.table);
    if (!table) return;

    std::unordered_set<std::string> provided;
    for (auto& c : ins.columns) provided.insert(to_upper(c));

    for (auto& col : table->columns) {
        if (!col.nullable && !col.is_pk && !provided.count(to_upper(col.name))) {
            emit(r, SemanticSeverity::WARNING, "MISSING_NOT_NULL_COLUMN",
                 "NOT NULL column '" + col.name + "' is not in INSERT column list",
                 "Table: " + ins.table);
        }
    }
}

// -----------------------------------------------------------------------
// Built-in rule: TYPE_MISMATCH_COMPARISON
// -----------------------------------------------------------------------
static DataType infer_literal_type(const ExprPtr& expr) {
    if (!expr || expr->type != ExprType::LITERAL) return DataType::UNKNOWN;
    const std::string& val = expr->value;
    if (val == "NULL" || val == "TRUE" || val == "FALSE") return DataType::BOOLEAN;

    // Try numeric
    bool has_dot = false;
    bool is_num = !val.empty();
    for (size_t i = 0; i < val.size(); ++i) {
        char c = val[i];
        if (c == '.' && !has_dot) { has_dot = true; continue; }
        if (c == '-' && i == 0) continue;
        if (!std::isdigit(static_cast<unsigned char>(c))) { is_num = false; break; }
    }
    if (is_num) return has_dot ? DataType::REAL : DataType::INTEGER;

    return DataType::TEXT; // string literal
}

void MeaningAnalyzer::rule_type_mismatch_comparison(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    if (!sel.where_clause || sel.from_table.empty()) return;

    auto* table = cat.find_table(sel.from_table);
    if (!table) return;

    // Simple check: binary comparison where one side is identifier, other is literal
    std::function<void(const ExprPtr&)> check_expr;
    check_expr = [&](const ExprPtr& expr) {
        if (!expr) return;

        if (expr->type == ExprType::BINARY_OP &&
            (expr->op == "=" || expr->op == "!=" || expr->op == "<>" ||
             expr->op == "<" || expr->op == ">" || expr->op == "<=" || expr->op == ">=")) {

            ExprPtr col_side, lit_side;
            if (expr->left->type == ExprType::IDENTIFIER &&
                expr->right->type == ExprType::LITERAL) {
                col_side = expr->left;
                lit_side = expr->right;
            } else if (expr->right->type == ExprType::IDENTIFIER &&
                       expr->left->type == ExprType::LITERAL) {
                col_side = expr->right;
                lit_side = expr->left;
            }

            if (col_side && lit_side) {
                std::string col_name = col_side->value;
                auto dot = col_name.find('.');
                if (dot != std::string::npos) col_name = col_name.substr(dot + 1);

                auto* col_info = table->find_column(col_name);
                if (col_info && col_info->type != DataType::UNKNOWN) {
                    DataType lit_type = infer_literal_type(lit_side);
                    if (lit_type != DataType::UNKNOWN && lit_type != col_info->type) {
                        // Allow integer/real cross-comparison
                        bool numeric_compat =
                            (col_info->type == DataType::INTEGER || col_info->type == DataType::REAL) &&
                            (lit_type == DataType::INTEGER || lit_type == DataType::REAL);
                        if (!numeric_compat) {
                            emit(r, SemanticSeverity::WARNING, "TYPE_MISMATCH_COMPARISON",
                                 "Comparing column '" + col_name + "' ("
                                 + data_type_name(col_info->type) + ") with "
                                 + data_type_name(lit_type) + " literal");
                        }
                    }
                }
            }
        }

        check_expr(expr->left);
        check_expr(expr->right);
        for (auto& a : expr->args) check_expr(a);
    };

    check_expr(sel.where_clause);
}

// -----------------------------------------------------------------------
// Built-in rule: AGGREGATE_IN_WHERE
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_aggregate_in_where(
    const Statement& s, const SchemaCatalog& /*cat*/, SemanticResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);

    if (sel.where_clause && expr_has_aggregate(sel.where_clause)) {
        emit(r, SemanticSeverity::ERROR, "AGGREGATE_IN_WHERE",
             "Aggregate function used in WHERE clause — use HAVING instead");
    }
}

// -----------------------------------------------------------------------
// Built-in rule: NON_AGGREGATED_COLUMN
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_non_aggregated_column(
    const Statement& s, const SchemaCatalog& /*cat*/, SemanticResult& r) {
    if (s.type != StmtType::SELECT) return;
    auto& sel = std::get<SelectStmt>(s.node);
    if (sel.group_by.empty()) return;

    // Collect GROUP BY column names
    std::unordered_set<std::string> grouped;
    for (auto& gb : sel.group_by) {
        if (gb->type == ExprType::IDENTIFIER) {
            grouped.insert(to_upper(gb->value));
        }
    }

    // Check each SELECT column
    for (auto& col : sel.columns) {
        if (col->type == ExprType::STAR) continue;
        if (expr_has_aggregate(col)) continue;

        if (col->type == ExprType::IDENTIFIER) {
            std::string name = to_upper(col->value);
            // Strip table qualifier
            auto dot = name.find('.');
            if (dot != std::string::npos) name = name.substr(dot + 1);

            if (!grouped.count(name)) {
                emit(r, SemanticSeverity::WARNING, "NON_AGGREGATED_COLUMN",
                     "Column '" + col->value + "' is not in GROUP BY and not aggregated");
            }
        }
    }
}

// -----------------------------------------------------------------------
// Built-in rule: MODIFY_NONEXISTENT_TABLE
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_modify_nonexistent_table(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {

    if (s.type == StmtType::UPDATE) {
        auto& upd = std::get<UpdateStmt>(s.node);
        if (!upd.table.empty() && !cat.has_table(upd.table)) {
            emit(r, SemanticSeverity::ERROR, "MODIFY_NONEXISTENT_TABLE",
                 "UPDATE target table '" + upd.table + "' does not exist");
        }
    } else if (s.type == StmtType::DELETE_STMT) {
        auto& del = std::get<DeleteStmt>(s.node);
        if (!del.table.empty() && !cat.has_table(del.table)) {
            emit(r, SemanticSeverity::ERROR, "MODIFY_NONEXISTENT_TABLE",
                 "DELETE target table '" + del.table + "' does not exist");
        }
    }
}

// -----------------------------------------------------------------------
// Built-in rule: DROP_NONEXISTENT_TABLE
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_drop_nonexistent_table(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {
    if (s.type != StmtType::DROP_TABLE) return;
    auto& dt = std::get<DropTableStmt>(s.node);

    if (!dt.if_exists && !dt.table.empty() && !cat.has_table(dt.table)) {
        emit(r, SemanticSeverity::ERROR, "DROP_NONEXISTENT_TABLE",
             "DROP TABLE '" + dt.table + "' — table does not exist (use IF EXISTS)");
    }
}

// -----------------------------------------------------------------------
// Built-in rule: CREATE_EXISTING_TABLE
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_create_existing_table(
    const Statement& s, const SchemaCatalog& cat, SemanticResult& r) {
    if (s.type != StmtType::CREATE_TABLE) return;
    auto& ct = std::get<CreateTableStmt>(s.node);

    if (!ct.if_not_exists && !ct.table.empty() && cat.has_table(ct.table)) {
        emit(r, SemanticSeverity::ERROR, "CREATE_EXISTING_TABLE",
             "Table '" + ct.table + "' already exists (use IF NOT EXISTS)");
    }
}

// -----------------------------------------------------------------------
// Built-in rule: DUPLICATE_COLUMN_DEF
// -----------------------------------------------------------------------
void MeaningAnalyzer::rule_duplicate_column_def(
    const Statement& s, const SchemaCatalog& /*cat*/, SemanticResult& r) {
    if (s.type != StmtType::CREATE_TABLE) return;
    auto& ct = std::get<CreateTableStmt>(s.node);

    std::unordered_set<std::string> seen;
    for (auto& col : ct.columns) {
        std::string upper = to_upper(col.name);
        if (seen.count(upper)) {
            emit(r, SemanticSeverity::ERROR, "DUPLICATE_COLUMN_DEF",
                 "Duplicate column name '" + col.name + "' in CREATE TABLE");
        }
        seen.insert(upper);
    }
}

} // namespace semantic
} // namespace sql
} // namespace ml
