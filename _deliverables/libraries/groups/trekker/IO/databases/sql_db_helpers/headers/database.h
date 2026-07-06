/**
 * Database Connection and Execution
 * 
 * Provides database connectivity and query execution.
 * Supports SQLite, PostgreSQL, and MySQL.
 */

#ifndef ML_SQL_DATABASE_H
#define ML_SQL_DATABASE_H

#include "schema_parser.h"
#include "query_builder.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace ml {
namespace sql {

// Query result row
using Row = std::map<std::string, std::string>;

// Query result set
struct ResultSet {
    std::vector<std::string> columns;
    std::vector<Row> rows;
    int affected_rows = 0;
    long long last_insert_id = 0;
    
    bool empty() const { return rows.empty(); }
    size_t size() const { return rows.size(); }
    
    // Get single row (throws if not exactly one row)
    Row get_single() const;
    
    // Get first row (returns empty map if no rows)
    Row get_first() const;
    
    // Convert to JSON string
    std::string to_json() const;
};

// Database connection interface
class Database {
protected:
    std::string connection_string_;
    std::string db_provider_;
    bool connected_ = false;
    
public:
    Database(const std::string& provider) : db_provider_(provider) {}
    virtual ~Database() = default;
    
    // Connection management
    virtual bool connect(const std::string& connection_string) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const { return connected_; }
    
    // Query execution
    virtual ResultSet execute(const std::string& query) = 0;
    virtual ResultSet execute(const std::string& query, const std::vector<std::string>& params) = 0;
    
    // Transaction support
    virtual bool begin_transaction() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
    
    // Schema operations
    virtual bool create_table_from_model(const Model& model) = 0;
    virtual bool drop_table(const std::string& table_name) = 0;
    
    // Utility
    std::string get_provider() const { return db_provider_; }
    
    // Factory method
    static std::unique_ptr<Database> create(const std::string& provider);
};

// SQLite implementation
class SQLiteDatabase : public Database {
private:
    void* db_handle_ = nullptr;  // sqlite3* handle
    
public:
    SQLiteDatabase();
    ~SQLiteDatabase() override;
    
    bool connect(const std::string& connection_string) override;
    void disconnect() override;
    
    ResultSet execute(const std::string& query) override;
    ResultSet execute(const std::string& query, const std::vector<std::string>& params) override;
    
    bool begin_transaction() override;
    bool commit() override;
    bool rollback() override;
    
    bool create_table_from_model(const Model& model) override;
    bool drop_table(const std::string& table_name) override;
};

// ORM-like interface for models
template<typename T>
class Repository {
private:
    Database& db_;
    Model model_;
    CRUDGenerator generator_;
    
public:
    Repository(Database& db, const Model& model)
        : db_(db), model_(model), generator_(model, db.get_provider()) {}
    
    // Find operations
    ResultSet find_all() {
        return db_.execute(generator_.generate_find_all());
    }
    
    ResultSet find_by_id(const std::string& id) {
        return db_.execute(generator_.generate_find_by_id(), {id});
    }
    
    ResultSet find_where(const std::map<std::string, std::string>& conditions) {
        return db_.execute(generator_.generate_find_where(conditions));
    }
    
    // Create operation
    ResultSet create(const std::map<std::string, std::string>& data) {
        return db_.execute(generator_.generate_insert(data));
    }
    
    // Update operations
    ResultSet update(const std::string& id, const std::map<std::string, std::string>& data) {
        return db_.execute(generator_.generate_update(id, data));
    }
    
    ResultSet update_where(const std::map<std::string, std::string>& conditions,
                          const std::map<std::string, std::string>& data) {
        return db_.execute(generator_.generate_update_where(conditions, data));
    }
    
    // Delete operations
    ResultSet remove(const std::string& id) {
        return db_.execute(generator_.generate_delete(id));
    }
    
    ResultSet remove_where(const std::map<std::string, std::string>& conditions) {
        return db_.execute(generator_.generate_delete_where(conditions));
    }
    
    // Schema operations
    bool create_table() {
        return db_.create_table_from_model(model_);
    }
    
    bool drop_table() {
        return db_.drop_table(model_.table_name);
    }
};

// Schema migrator
class Migrator {
private:
    Database& db_;
    Schema schema_;
    
public:
    Migrator(Database& db, const Schema& schema) : db_(db), schema_(schema) {}
    
    // Create all tables from schema
    bool migrate_up();
    
    // Drop all tables
    bool migrate_down();
    
    // Reset database (down then up)
    bool reset();
    
    // Generate migration SQL
    std::string generate_migration_sql() const;
};

} // namespace sql
} // namespace ml

#endif // ML_SQL_DATABASE_H
