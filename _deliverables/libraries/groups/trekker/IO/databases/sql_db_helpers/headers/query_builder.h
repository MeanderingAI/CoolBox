/**
 * SQL Query Builder
 * 
 * Fluent API for building SQL queries with type safety.
 * Generates CRUD operations from Prisma schema models.
 */

#ifndef ML_SQL_QUERY_BUILDER_H
#define ML_SQL_QUERY_BUILDER_H

#include "schema_parser.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace ml {
namespace sql {

// Query types
enum class QueryType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE_TABLE,
    DROP_TABLE,
    ALTER_TABLE
};

// Where condition operators
enum class Operator {
    EQUALS,
    NOT_EQUALS,
    GREATER_THAN,
    GREATER_EQUAL,
    LESS_THAN,
    LESS_EQUAL,
    LIKE,
    IN,
    NOT_IN,
    IS_NULL,
    IS_NOT_NULL,
    BETWEEN
};

// Order direction
enum class OrderDirection {
    ASC,
    DESC
};

// Where condition
struct WhereCondition {
    std::string field;
    Operator op;
    std::string value;
    std::string value2;  // For BETWEEN
    
    std::string to_sql() const;
};

// Query builder with fluent API
class QueryBuilder {
private:
    QueryType type_;
    std::string table_;
    std::vector<std::string> columns_;
    std::map<std::string, std::string> values_;
    std::vector<WhereCondition> conditions_;
    std::vector<std::pair<std::string, OrderDirection>> order_by_;
    int limit_ = -1;
    int offset_ = -1;
    std::vector<std::string> joins_;
    
public:
    QueryBuilder(QueryType type, const std::string& table);
    
    // SELECT operations
    QueryBuilder& select(const std::vector<std::string>& columns);
    QueryBuilder& select(const std::string& column);
    QueryBuilder& select_all();
    
    // WHERE conditions
    QueryBuilder& where(const std::string& field, Operator op, const std::string& value);
    QueryBuilder& where(const std::string& field, const std::string& value);  // Defaults to EQUALS
    QueryBuilder& where_in(const std::string& field, const std::vector<std::string>& values);
    QueryBuilder& where_between(const std::string& field, const std::string& low, const std::string& high);
    QueryBuilder& where_null(const std::string& field);
    QueryBuilder& where_not_null(const std::string& field);
    
    // INSERT operations
    QueryBuilder& insert(const std::map<std::string, std::string>& data);
    QueryBuilder& set(const std::string& field, const std::string& value);
    
    // UPDATE operations
    QueryBuilder& update(const std::map<std::string, std::string>& data);
    
    // ORDER BY
    QueryBuilder& order_by(const std::string& field, OrderDirection dir = OrderDirection::ASC);
    
    // LIMIT and OFFSET
    QueryBuilder& limit(int limit);
    QueryBuilder& offset(int offset);
    
    // JOIN operations
    QueryBuilder& join(const std::string& table, const std::string& condition);
    QueryBuilder& left_join(const std::string& table, const std::string& condition);
    
    // Build final query
    std::string build() const;
    
    // Get parameter values for prepared statements
    std::vector<std::string> get_params() const;
};

// CRUD operation generator from Prisma models
class CRUDGenerator {
private:
    const Model& model_;
    std::string db_provider_;
    
public:
    CRUDGenerator(const Model& model, const std::string& db_provider = "sqlite");
    
    // Generate CREATE TABLE statement
    std::string generate_create_table() const;
    
    // Generate DROP TABLE statement
    std::string generate_drop_table() const;
    
    // Generate SELECT queries
    std::string generate_find_all() const;
    std::string generate_find_by_id(const std::string& id_placeholder = "?") const;
    std::string generate_find_where(const std::map<std::string, std::string>& conditions) const;
    
    // Generate INSERT query
    std::string generate_insert(const std::map<std::string, std::string>& data) const;
    
    // Generate UPDATE query
    std::string generate_update(const std::string& id, const std::map<std::string, std::string>& data) const;
    std::string generate_update_where(const std::map<std::string, std::string>& conditions,
                                     const std::map<std::string, std::string>& data) const;
    
    // Generate DELETE query
    std::string generate_delete(const std::string& id) const;
    std::string generate_delete_where(const std::map<std::string, std::string>& conditions) const;
    
    // Generate all CRUD operations as SQL script
    std::string generate_all_operations() const;
};

// Helper functions
std::string operator_to_sql(Operator op);
std::string escape_value(const std::string& value);
std::string quote_identifier(const std::string& identifier, const std::string& db_provider = "sqlite");

} // namespace sql
} // namespace ml

#endif // ML_SQL_QUERY_BUILDER_H
