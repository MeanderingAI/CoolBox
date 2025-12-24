#include "databases/sql/database.h"
#include "dataformats/json/json.h"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>

namespace ml {
namespace sql {

// =========================================================================
// ResultSet Implementation
// =========================================================================

Row ResultSet::get_single() const {
    if (rows.empty()) {
        throw std::runtime_error("No rows in result set");
    }
    if (rows.size() > 1) {
        throw std::runtime_error("Expected single row, got " + std::to_string(rows.size()));
    }
    return rows[0];
}

Row ResultSet::get_first() const {
    if (rows.empty()) {
        return Row{};
    }
    return rows[0];
}

std::string ResultSet::to_json() const {
    dataformats::json::Array rows_array;
    
    for (const auto& row : rows) {
        dataformats::json::Builder row_builder;
        for (const auto& [key, value] : row) {
            row_builder.add(key, value);
        }
        rows_array.push(row_builder.build());
    }
    
    dataformats::json::Builder result_builder;
    result_builder.add("count", std::to_string(rows.size()))
                 .add("rows", dataformats::json::Value(rows_array));
    
    return result_builder.build().to_string();
}

// =========================================================================
// Database Factory
// =========================================================================

std::unique_ptr<Database> Database::create(const std::string& provider) {
    if (provider == "sqlite") {
        return std::make_unique<SQLiteDatabase>();
    }
    
    throw std::runtime_error("Unsupported database provider: " + provider);
}

// =========================================================================
// SQLiteDatabase Implementation
// =========================================================================

SQLiteDatabase::SQLiteDatabase() : Database("sqlite") {}

SQLiteDatabase::~SQLiteDatabase() {
    disconnect();
}

bool SQLiteDatabase::connect(const std::string& connection_string) {
    if (connected_) {
        disconnect();
    }
    
    connection_string_ = connection_string;
    
    int rc = sqlite3_open(connection_string.c_str(), 
                         reinterpret_cast<sqlite3**>(&db_handle_));
    
    if (rc != SQLITE_OK) {
        sqlite3_close(reinterpret_cast<sqlite3*>(db_handle_));
        db_handle_ = nullptr;
        return false;
    }
    
    connected_ = true;
    return true;
}

void SQLiteDatabase::disconnect() {
    if (db_handle_) {
        sqlite3_close(reinterpret_cast<sqlite3*>(db_handle_));
        db_handle_ = nullptr;
    }
    connected_ = false;
}

ResultSet SQLiteDatabase::execute(const std::string& query) {
    if (!connected_ || !db_handle_) {
        throw std::runtime_error("Database not connected");
    }
    
    ResultSet result;
    sqlite3* db = reinterpret_cast<sqlite3*>(db_handle_);
    sqlite3_stmt* stmt = nullptr;
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        throw std::runtime_error("SQL prepare error: " + error);
    }
    
    // Get column names
    int col_count = sqlite3_column_count(stmt);
    for (int i = 0; i < col_count; ++i) {
        result.columns.push_back(sqlite3_column_name(stmt, i));
    }
    
    // Fetch rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        for (int i = 0; i < col_count; ++i) {
            const char* value = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, i));
            row[result.columns[i]] = value ? value : "";
        }
        result.rows.push_back(row);
    }
    
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQL execution error: " + error);
    }
    
    result.affected_rows = sqlite3_changes(db);
    result.last_insert_id = sqlite3_last_insert_rowid(db);
    
    sqlite3_finalize(stmt);
    return result;
}

ResultSet SQLiteDatabase::execute(const std::string& query, 
                                  const std::vector<std::string>& params) {
    if (!connected_ || !db_handle_) {
        throw std::runtime_error("Database not connected");
    }
    
    ResultSet result;
    sqlite3* db = reinterpret_cast<sqlite3*>(db_handle_);
    sqlite3_stmt* stmt = nullptr;
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        throw std::runtime_error("SQL prepare error: " + error);
    }
    
    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i) {
        rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throw std::runtime_error("Parameter binding error: " + error);
        }
    }
    
    // Get column names
    int col_count = sqlite3_column_count(stmt);
    for (int i = 0; i < col_count; ++i) {
        result.columns.push_back(sqlite3_column_name(stmt, i));
    }
    
    // Fetch rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        for (int i = 0; i < col_count; ++i) {
            const char* value = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, i));
            row[result.columns[i]] = value ? value : "";
        }
        result.rows.push_back(row);
    }
    
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQL execution error: " + error);
    }
    
    result.affected_rows = sqlite3_changes(db);
    result.last_insert_id = sqlite3_last_insert_rowid(db);
    
    sqlite3_finalize(stmt);
    return result;
}

bool SQLiteDatabase::begin_transaction() {
    try {
        execute("BEGIN TRANSACTION");
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SQLiteDatabase::commit() {
    try {
        execute("COMMIT");
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SQLiteDatabase::rollback() {
    try {
        execute("ROLLBACK");
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SQLiteDatabase::create_table_from_model(const Model& model) {
    try {
        CRUDGenerator generator(model, db_provider_);
        std::string create_sql = generator.generate_create_table();
        execute(create_sql);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SQLiteDatabase::drop_table(const std::string& table_name) {
    try {
        execute("DROP TABLE IF EXISTS " + table_name);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// =========================================================================
// Migrator Implementation
// =========================================================================

bool Migrator::migrate_up() {
    bool success = true;
    
    if (!db_.begin_transaction()) {
        return false;
    }
    
    for (const auto& model : schema_.models()) {
        if (!db_.create_table_from_model(model)) {
            db_.rollback();
            return false;
        }
    }
    
    return db_.commit();
}

bool Migrator::migrate_down() {
    bool success = true;
    
    if (!db_.begin_transaction()) {
        return false;
    }
    
    for (const auto& model : schema_.models()) {
        if (!db_.drop_table(model.table_name)) {
            db_.rollback();
            return false;
        }
    }
    
    return db_.commit();
}

bool Migrator::reset() {
    return migrate_down() && migrate_up();
}

std::string Migrator::generate_migration_sql() const {
    std::ostringstream oss;
    
    oss << "-- Generated Migration SQL\n";
    oss << "-- Database: " << schema_.get_provider() << "\n\n";
    
    for (const auto& model : schema_.models()) {
        CRUDGenerator generator(model, schema_.get_provider());
        oss << generator.generate_create_table() << ";\n\n";
    }
    
    return oss.str();
}

} // namespace sql
} // namespace ml
