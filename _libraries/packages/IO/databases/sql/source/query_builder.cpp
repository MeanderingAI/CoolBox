#include "IO/databases/sql/query_builder.h"
#include <sstream>
#include <algorithm>

namespace ml {
namespace sql {

// =========================================================================
// WhereCondition Implementation
// =========================================================================

std::string WhereCondition::to_sql() const {
    std::ostringstream oss;
    oss << field << " " << operator_to_sql(op);
    
    switch (op) {
        case Operator::IS_NULL:
        case Operator::IS_NOT_NULL:
            // No value needed
            break;
        case Operator::BETWEEN:
            oss << " " << value << " AND " << value2;
            break;
        case Operator::IN:
        case Operator::NOT_IN:
            oss << " (" << value << ")";
            break;
        default:
            oss << " " << value;
            break;
    }
    
    return oss.str();
}

// =========================================================================
// QueryBuilder Implementation
// =========================================================================

QueryBuilder::QueryBuilder(QueryType type, const std::string& table)
    : type_(type), table_(table) {}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
    columns_ = columns;
    return *this;
}

QueryBuilder& QueryBuilder::select(const std::string& column) {
    columns_.push_back(column);
    return *this;
}

QueryBuilder& QueryBuilder::select_all() {
    columns_ = {"*"};
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& field, Operator op, const std::string& value) {
    WhereCondition cond;
    cond.field = field;
    cond.op = op;
    cond.value = escape_value(value);
    conditions_.push_back(cond);
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& field, const std::string& value) {
    return where(field, Operator::EQUALS, value);
}

QueryBuilder& QueryBuilder::where_in(const std::string& field, const std::vector<std::string>& values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << escape_value(values[i]);
    }
    
    WhereCondition cond;
    cond.field = field;
    cond.op = Operator::IN;
    cond.value = oss.str();
    conditions_.push_back(cond);
    return *this;
}

QueryBuilder& QueryBuilder::where_between(const std::string& field, const std::string& low, const std::string& high) {
    WhereCondition cond;
    cond.field = field;
    cond.op = Operator::BETWEEN;
    cond.value = escape_value(low);
    cond.value2 = escape_value(high);
    conditions_.push_back(cond);
    return *this;
}

QueryBuilder& QueryBuilder::where_null(const std::string& field) {
    WhereCondition cond;
    cond.field = field;
    cond.op = Operator::IS_NULL;
    conditions_.push_back(cond);
    return *this;
}

QueryBuilder& QueryBuilder::where_not_null(const std::string& field) {
    WhereCondition cond;
    cond.field = field;
    cond.op = Operator::IS_NOT_NULL;
    conditions_.push_back(cond);
    return *this;
}

QueryBuilder& QueryBuilder::insert(const std::map<std::string, std::string>& data) {
    values_ = data;
    return *this;
}

QueryBuilder& QueryBuilder::set(const std::string& field, const std::string& value) {
    values_[field] = value;
    return *this;
}

QueryBuilder& QueryBuilder::update(const std::map<std::string, std::string>& data) {
    values_ = data;
    return *this;
}

QueryBuilder& QueryBuilder::order_by(const std::string& field, OrderDirection dir) {
    order_by_.emplace_back(field, dir);
    return *this;
}

QueryBuilder& QueryBuilder::limit(int limit) {
    limit_ = limit;
    return *this;
}

QueryBuilder& QueryBuilder::offset(int offset) {
    offset_ = offset;
    return *this;
}

QueryBuilder& QueryBuilder::join(const std::string& table, const std::string& condition) {
    joins_.push_back("JOIN " + table + " ON " + condition);
    return *this;
}

QueryBuilder& QueryBuilder::left_join(const std::string& table, const std::string& condition) {
    joins_.push_back("LEFT JOIN " + table + " ON " + condition);
    return *this;
}

std::string QueryBuilder::build() const {
    std::ostringstream oss;
    
    switch (type_) {
        case QueryType::SELECT: {
            oss << "SELECT ";
            if (columns_.empty()) {
                oss << "*";
            } else {
                for (size_t i = 0; i < columns_.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << columns_[i];
                }
            }
            oss << " FROM " << table_;
            
            // JOINs
            for (const auto& join : joins_) {
                oss << " " << join;
            }
            
            // WHERE
            if (!conditions_.empty()) {
                oss << " WHERE ";
                for (size_t i = 0; i < conditions_.size(); ++i) {
                    if (i > 0) oss << " AND ";
                    oss << conditions_[i].to_sql();
                }
            }
            
            // ORDER BY
            if (!order_by_.empty()) {
                oss << " ORDER BY ";
                for (size_t i = 0; i < order_by_.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << order_by_[i].first << " ";
                    oss << (order_by_[i].second == OrderDirection::ASC ? "ASC" : "DESC");
                }
            }
            
            // LIMIT
            if (limit_ > 0) {
                oss << " LIMIT " << limit_;
            }
            
            // OFFSET
            if (offset_ > 0) {
                oss << " OFFSET " << offset_;
            }
            break;
        }
        
        case QueryType::INSERT: {
            oss << "INSERT INTO " << table_ << " (";
            
            std::vector<std::string> fields;
            std::vector<std::string> vals;
            
            for (const auto& [field, value] : values_) {
                fields.push_back(field);
                vals.push_back(escape_value(value));
            }
            
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << fields[i];
            }
            
            oss << ") VALUES (";
            
            for (size_t i = 0; i < vals.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << vals[i];
            }
            
            oss << ")";
            break;
        }
        
        case QueryType::UPDATE: {
            oss << "UPDATE " << table_ << " SET ";
            
            size_t i = 0;
            for (const auto& [field, value] : values_) {
                if (i > 0) oss << ", ";
                oss << field << " = " << escape_value(value);
                i++;
            }
            
            // WHERE
            if (!conditions_.empty()) {
                oss << " WHERE ";
                for (size_t j = 0; j < conditions_.size(); ++j) {
                    if (j > 0) oss << " AND ";
                    oss << conditions_[j].to_sql();
                }
            }
            break;
        }
        
        case QueryType::DELETE: {
            oss << "DELETE FROM " << table_;
            
            // WHERE
            if (!conditions_.empty()) {
                oss << " WHERE ";
                for (size_t i = 0; i < conditions_.size(); ++i) {
                    if (i > 0) oss << " AND ";
                    oss << conditions_[i].to_sql();
                }
            }
            break;
        }
        
        default:
            break;
    }
    
    return oss.str();
}

std::vector<std::string> QueryBuilder::get_params() const {
    std::vector<std::string> params;
    for (const auto& [_, value] : values_) {
        params.push_back(value);
    }
    return params;
}

// =========================================================================
// CRUDGenerator Implementation
// =========================================================================

CRUDGenerator::CRUDGenerator(const Model& model, const std::string& db_provider)
    : model_(model), db_provider_(db_provider) {}

std::string CRUDGenerator::generate_create_table() const {
    std::ostringstream oss;
    oss << "CREATE TABLE " << model_.table_name << " (\n";
    
    auto data_fields = model_.get_data_fields();
    for (size_t i = 0; i < data_fields.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << "  " << field_to_sql_definition(data_fields[i], db_provider_);
    }
    
    oss << "\n)";
    return oss.str();
}

std::string CRUDGenerator::generate_drop_table() const {
    return "DROP TABLE IF EXISTS " + model_.table_name;
}

std::string CRUDGenerator::generate_find_all() const {
    QueryBuilder builder(QueryType::SELECT, model_.table_name);
    builder.select_all();
    return builder.build();
}

std::string CRUDGenerator::generate_find_by_id(const std::string& id_placeholder) const {
    auto pks = model_.get_primary_keys();
    if (pks.empty()) {
        throw std::runtime_error("Model has no primary key");
    }
    
    QueryBuilder builder(QueryType::SELECT, model_.table_name);
    builder.select_all().where(pks[0].name, id_placeholder);
    return builder.build();
}

std::string CRUDGenerator::generate_find_where(const std::map<std::string, std::string>& conditions) const {
    QueryBuilder builder(QueryType::SELECT, model_.table_name);
    builder.select_all();
    
    for (const auto& [field, value] : conditions) {
        builder.where(field, value);
    }
    
    return builder.build();
}

std::string CRUDGenerator::generate_insert(const std::map<std::string, std::string>& data) const {
    QueryBuilder builder(QueryType::INSERT, model_.table_name);
    builder.insert(data);
    return builder.build();
}

std::string CRUDGenerator::generate_update(const std::string& id, const std::map<std::string, std::string>& data) const {
    auto pks = model_.get_primary_keys();
    if (pks.empty()) {
        throw std::runtime_error("Model has no primary key");
    }
    
    QueryBuilder builder(QueryType::UPDATE, model_.table_name);
    builder.update(data).where(pks[0].name, id);
    return builder.build();
}

std::string CRUDGenerator::generate_update_where(const std::map<std::string, std::string>& conditions,
                                                 const std::map<std::string, std::string>& data) const {
    QueryBuilder builder(QueryType::UPDATE, model_.table_name);
    builder.update(data);
    
    for (const auto& [field, value] : conditions) {
        builder.where(field, value);
    }
    
    return builder.build();
}

std::string CRUDGenerator::generate_delete(const std::string& id) const {
    auto pks = model_.get_primary_keys();
    if (pks.empty()) {
        throw std::runtime_error("Model has no primary key");
    }
    
    QueryBuilder builder(QueryType::DELETE, model_.table_name);
    builder.where(pks[0].name, id);
    return builder.build();
}

std::string CRUDGenerator::generate_delete_where(const std::map<std::string, std::string>& conditions) const {
    QueryBuilder builder(QueryType::DELETE, model_.table_name);
    
    for (const auto& [field, value] : conditions) {
        builder.where(field, value);
    }
    
    return builder.build();
}

std::string CRUDGenerator::generate_all_operations() const {
    std::ostringstream oss;
    
    oss << "-- CRUD Operations for " << model_.name << "\n\n";
    
    oss << "-- Create Table\n";
    oss << generate_create_table() << ";\n\n";
    
    oss << "-- Find All\n";
    oss << generate_find_all() << ";\n\n";
    
    oss << "-- Find By ID\n";
    oss << generate_find_by_id("?") << ";\n\n";
    
    oss << "-- Insert\n";
    oss << "-- " << generate_insert({{"field1", "value1"}}) << ";\n\n";
    
    oss << "-- Update\n";
    oss << "-- " << generate_update("?", {{"field1", "value1"}}) << ";\n\n";
    
    oss << "-- Delete\n";
    oss << generate_delete("?") << ";\n\n";
    
    return oss.str();
}

// =========================================================================
// Helper Functions
// =========================================================================

std::string operator_to_sql(Operator op) {
    switch (op) {
        case Operator::EQUALS: return "=";
        case Operator::NOT_EQUALS: return "!=";
        case Operator::GREATER_THAN: return ">";
        case Operator::GREATER_EQUAL: return ">=";
        case Operator::LESS_THAN: return "<";
        case Operator::LESS_EQUAL: return "<=";
        case Operator::LIKE: return "LIKE";
        case Operator::IN: return "IN";
        case Operator::NOT_IN: return "NOT IN";
        case Operator::IS_NULL: return "IS NULL";
        case Operator::IS_NOT_NULL: return "IS NOT NULL";
        case Operator::BETWEEN: return "BETWEEN";
    }
    return "=";
}

std::string escape_value(const std::string& value) {
    if (value.empty()) return "''";
    
    // Check if already quoted or is a number
    if (value[0] == '\'' || value[0] == '"') return value;
    
    // Check if it's a number
    bool is_number = true;
    for (char c : value) {
        if (!std::isdigit(c) && c != '.' && c != '-') {
            is_number = false;
            break;
        }
    }
    
    if (is_number) return value;
    
    // Escape single quotes and wrap in quotes
    std::string escaped;
    for (char c : value) {
        if (c == '\'') escaped += "''";
        else escaped += c;
    }
    
    return "'" + escaped + "'";
}

std::string quote_identifier(const std::string& identifier, const std::string& db_provider) {
    if (db_provider == "postgresql") {
        return "\"" + identifier + "\"";
    } else if (db_provider == "mysql") {
        return "`" + identifier + "`";
    }
    // SQLite accepts both
    return identifier;
}

} // namespace sql
} // namespace ml
