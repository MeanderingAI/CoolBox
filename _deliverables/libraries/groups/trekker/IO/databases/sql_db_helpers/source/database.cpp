#include "../headers/database.h"
#include "json.h"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <cctype>
#include <algorithm>
#include <cstdint>

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

namespace {

struct MigrationFiles {
    std::string version;
    std::string name;
    std::string up_path;
    std::string down_path;
    std::string up_sql;
    std::string down_sql;
    std::string checksum;
};

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("could not open migration file: " + path);
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) throw std::runtime_error("failed while reading migration file: " + path);
    return contents.str();
}

std::string migration_checksum(const std::string& content) {
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char byte : content) {
        value ^= byte;
        value *= 1099511628211ULL;
    }
    std::ostringstream result;
    result << std::hex << std::setw(16) << std::setfill('0') << value;
    return result.str();
}

std::vector<std::string> split_sql_script(const std::string& script) {
    std::vector<std::string> statements;
    std::string current;
    char quote = '\0';
    bool line_comment = false;
    bool block_comment = false;
    std::string dollar_quote;

    for (std::size_t i = 0; i < script.size(); ++i) {
        const char ch = script[i];
        const char next = i + 1 < script.size() ? script[i + 1] : '\0';
        if (line_comment) {
            current.push_back(ch);
            if (ch == '\n') line_comment = false;
            continue;
        }
        if (block_comment) {
            current.push_back(ch);
            if (ch == '*' && next == '/') {
                current.push_back(script[++i]);
                block_comment = false;
            }
            continue;
        }
        if (!dollar_quote.empty()) {
            if (script.compare(i, dollar_quote.size(), dollar_quote) == 0) {
                current += dollar_quote;
                i += dollar_quote.size() - 1;
                dollar_quote.clear();
            } else {
                current.push_back(ch);
            }
            continue;
        }
        if (quote != '\0') {
            current.push_back(ch);
            if (ch == '\\' && i + 1 < script.size()) {
                current.push_back(script[++i]);
            } else if (ch == quote) {
                if (next == quote) current.push_back(script[++i]);
                else quote = '\0';
            }
            continue;
        }
        if (ch == '-' && next == '-') {
            current += "--";
            ++i;
            line_comment = true;
        } else if (ch == '/' && next == '*') {
            current += "/*";
            ++i;
            block_comment = true;
        } else if (ch == '\'' || ch == '"' || ch == '`') {
            quote = ch;
            current.push_back(ch);
        } else if (ch == '$') {
            const auto tag_end = script.find('$', i + 1);
            if (tag_end != std::string::npos &&
                script.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", i + 1) >= tag_end) {
                dollar_quote = script.substr(i, tag_end - i + 1);
                current += dollar_quote;
                i = tag_end;
            } else {
                current.push_back(ch);
            }
        } else if (ch == ';') {
            const auto first = current.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) statements.push_back(current.substr(first));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    const auto first = current.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) statements.push_back(current.substr(first));
    if (quote != '\0' || block_comment || !dollar_quote.empty()) {
        throw std::runtime_error("unterminated quote or comment in migration SQL");
    }
    return statements;
}

bool has_sql_token(const std::string& statement) {
    bool line_comment = false;
    bool block_comment = false;
    for (std::size_t i = 0; i < statement.size(); ++i) {
        const char ch = statement[i];
        const char next = i + 1 < statement.size() ? statement[i + 1] : '\0';
        if (line_comment) {
            if (ch == '\n') line_comment = false;
        } else if (block_comment) {
            if (ch == '*' && next == '/') {
                ++i;
                block_comment = false;
            }
        } else if (ch == '-' && next == '-') {
            ++i;
            line_comment = true;
        } else if (ch == '/' && next == '*') {
            ++i;
            block_comment = true;
        } else if (!std::isspace(static_cast<unsigned char>(ch))) {
            return true;
        }
    }
    return false;
}

std::vector<MigrationFiles> load_migrations(const std::string& directory) {
    namespace fs = std::filesystem;
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        throw std::runtime_error("migration directory does not exist: " + directory);
    }
    std::map<std::string, MigrationFiles> by_version;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        const std::string filename = entry.path().filename().string();
        const bool is_up = filename.size() > 7 &&
            filename.compare(filename.size() - 7, 7, ".up.sql") == 0;
        const bool is_down = filename.size() > 9 &&
            filename.compare(filename.size() - 9, 9, ".down.sql") == 0;
        if (!is_up && !is_down) continue;
        const std::size_t suffix_size = is_up ? 7 : 9;
        const std::string stem = filename.substr(0, filename.size() - suffix_size);
        const auto separator = stem.find('_');
        if (separator == std::string::npos || separator == 0 || separator + 1 == stem.size()) {
            throw std::runtime_error("migration filename must be VERSION_name.up.sql or VERSION_name.down.sql: " +
                                     filename);
        }
        const std::string version = stem.substr(0, separator);
        if (!std::all_of(version.begin(), version.end(),
                         [](unsigned char value) { return std::isdigit(value); })) {
            throw std::runtime_error("migration version must contain only digits: " + filename);
        }
        auto& migration = by_version[version];
        if (migration.version.empty()) {
            migration.version = version;
            migration.name = stem.substr(separator + 1);
        } else if (migration.name != stem.substr(separator + 1)) {
            throw std::runtime_error("migration version is used by multiple names: " + version);
        }
        std::string& path = is_up ? migration.up_path : migration.down_path;
        if (!path.empty()) throw std::runtime_error("duplicate migration file: " + filename);
        path = entry.path().string();
    }

    std::vector<MigrationFiles> migrations;
    for (auto& [_, migration] : by_version) {
        if (migration.up_path.empty() || migration.down_path.empty()) {
            throw std::runtime_error("migration " + migration.version +
                                     " must have both .up.sql and .down.sql files");
        }
        migration.up_sql = read_file(migration.up_path);
        migration.down_sql = read_file(migration.down_path);
        migration.checksum = migration_checksum(migration.up_sql + "\n-- DOWN --\n" + migration.down_sql);
        migrations.push_back(std::move(migration));
    }
    return migrations;
}

void execute_script(Database& db, const std::string& script) {
    for (const auto& statement : split_sql_script(script)) {
        if (has_sql_token(statement)) db.execute(statement);
    }
}

} // namespace

// =========================================================================
// Database Factory
// =========================================================================

std::unique_ptr<Database> Database::create(const std::string& provider) {
    if (provider == "sqlite") {
        return std::make_unique<SQLiteDatabase>();
    }
#ifdef COOLBOX_SQL_ENABLE_POSTGRESQL
    if (provider == "postgresql" || provider == "postgres") {
        return std::make_unique<PostgreSQLDatabase>();
    }
#endif
#ifdef COOLBOX_SQL_ENABLE_MYSQL
    if (provider == "mysql" || provider == "mariadb") {
        return std::make_unique<MySQLDatabase>();
    }
#endif
    
    throw std::runtime_error("Unsupported or disabled database provider: " + provider);
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
        last_error_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_handle_));
        sqlite3_close(reinterpret_cast<sqlite3*>(db_handle_));
        db_handle_ = nullptr;
        return false;
    }
    
    connected_ = true;
    last_error_.clear();
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

void MigrationRunner::ensure_history_table() {
    db_.execute(
        "CREATE TABLE IF NOT EXISTS _coolbox_migrations ("
        "version VARCHAR(64) PRIMARY KEY, "
        "name VARCHAR(255) NOT NULL, "
        "checksum VARCHAR(16) NOT NULL, "
        "applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)");
}

std::string MigrationRunner::current_version() {
    ensure_history_table();
    const auto applied = db_.execute(
        "SELECT version FROM _coolbox_migrations ORDER BY version DESC LIMIT 1");
    return applied.empty() ? "" : applied.rows.front().at("version");
}

void MigrationRunner::migrate_up() {
    ensure_history_table();
    const auto migrations = load_migrations(directory_);
    const auto applied = db_.execute("SELECT version, checksum FROM _coolbox_migrations");
    std::map<std::string, std::string> applied_checksums;
    for (const auto& row : applied.rows) {
        applied_checksums[row.at("version")] = row.at("checksum");
    }
    bool missing_earlier_version = false;
    for (const auto& migration : migrations) {
        const auto already_applied = applied_checksums.find(migration.version);
        if (already_applied == applied_checksums.end()) {
            missing_earlier_version = true;
        } else if (missing_earlier_version) {
            throw std::runtime_error(
                "cannot apply an earlier migration after a later version: " + migration.version);
        }
    }
    for (const auto& [version, checksum] : applied_checksums) {
        const auto migration = std::find_if(migrations.begin(), migrations.end(),
            [&version](const MigrationFiles& item) { return item.version == version; });
        if (migration == migrations.end()) {
            throw std::runtime_error("missing migration files for applied version " + version);
        }
        if (migration->checksum != checksum) {
            throw std::runtime_error("applied migration was modified: " + version);
        }
    }
    for (const auto& migration : migrations) {
        const auto already_applied = applied_checksums.find(migration.version);
        if (already_applied != applied_checksums.end()) {
            if (already_applied->second != migration.checksum) {
                throw std::runtime_error("applied migration was modified: " + migration.version);
            }
            continue;
        }
        if (!db_.begin_transaction()) {
            throw std::runtime_error("could not begin migration transaction");
        }
        try {
            execute_script(db_, migration.up_sql);
            db_.execute(
                "INSERT INTO _coolbox_migrations (version, name, checksum) VALUES (?, ?, ?)",
                {migration.version, migration.name, migration.checksum});
            if (!db_.commit()) throw std::runtime_error("could not commit migration " + migration.version);
        } catch (...) {
            db_.rollback();
            throw;
        }
    }
}

void MigrationRunner::migrate_down(size_t steps) {
    if (steps == 0) return;
    ensure_history_table();
    const auto migrations = load_migrations(directory_);
    std::map<std::string, MigrationFiles> by_version;
    for (const auto& migration : migrations) by_version.emplace(migration.version, migration);
    const auto applied = db_.execute(
        "SELECT version, checksum FROM _coolbox_migrations ORDER BY version DESC");
    std::size_t reverted = 0;
    for (const auto& row : applied.rows) {
        if (reverted >= steps) break;
        const auto migration = by_version.find(row.at("version"));
        if (migration == by_version.end()) {
            throw std::runtime_error("missing migration files for applied version " + row.at("version"));
        }
        if (migration->second.checksum != row.at("checksum")) {
            throw std::runtime_error("applied migration was modified: " + row.at("version"));
        }
        if (!db_.begin_transaction()) {
            throw std::runtime_error("could not begin migration rollback transaction");
        }
        try {
            execute_script(db_, migration->second.down_sql);
            db_.execute("DELETE FROM _coolbox_migrations WHERE version = ?", {row.at("version")});
            if (!db_.commit()) {
                throw std::runtime_error("could not commit rollback for migration " + row.at("version"));
            }
        } catch (...) {
            db_.rollback();
            throw;
        }
        ++reverted;
    }
}

} // namespace sql
} // namespace ml
