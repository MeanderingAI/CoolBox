#include "../headers/database.h"
#include <libpq-fe.h>
#include <stdexcept>

namespace ml {
namespace sql {
namespace {

PGconn* connection(void* handle) {
    return static_cast<PGconn*>(handle);
}

void check_result(PGconn* db, PGresult* result) {
    if (!result) {
        throw std::runtime_error(std::string("PostgreSQL query failed: ") + PQerrorMessage(db));
    }
    const auto status = PQresultStatus(result);
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        const std::string error = PQresultErrorMessage(result);
        PQclear(result);
        throw std::runtime_error("PostgreSQL query failed: " + error);
    }
}

ResultSet read_result(PGconn* db, PGresult* pg_result) {
    check_result(db, pg_result);
    ResultSet result;
    const int column_count = PQnfields(pg_result);
    for (int column = 0; column < column_count; ++column) {
        result.columns.emplace_back(PQfname(pg_result, column));
    }
    const int row_count = PQntuples(pg_result);
    for (int row_index = 0; row_index < row_count; ++row_index) {
        Row row;
        for (int column = 0; column < column_count; ++column) {
            row[result.columns[column]] = PQgetisnull(pg_result, row_index, column)
                ? "" : PQgetvalue(pg_result, row_index, column);
        }
        result.rows.push_back(std::move(row));
    }
    const char* affected_rows = PQcmdTuples(pg_result);
    if (affected_rows && *affected_rows) {
        result.affected_rows = std::stoi(affected_rows);
    }
    PQclear(pg_result);
    return result;
}

std::string numbered_placeholders(const std::string& query) {
    std::string result;
    result.reserve(query.size());
    bool in_single_quote = false;
    bool in_double_quote = false;
    std::size_t parameter_index = 0;
    for (std::size_t i = 0; i < query.size(); ++i) {
        const char ch = query[i];
        if (ch == '\'' && !in_double_quote) {
            result.push_back(ch);
            if (in_single_quote && i + 1 < query.size() && query[i + 1] == '\'') {
                result.push_back(query[++i]);
            } else {
                in_single_quote = !in_single_quote;
            }
        } else if (ch == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            result.push_back(ch);
        } else if (ch == '?' && !in_single_quote && !in_double_quote) {
            result += "$" + std::to_string(++parameter_index);
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

} // namespace

PostgreSQLDatabase::PostgreSQLDatabase() : Database("postgresql") {}

PostgreSQLDatabase::~PostgreSQLDatabase() {
    disconnect();
}

bool PostgreSQLDatabase::connect(const std::string& connection_string) {
    disconnect();
    connection_string_ = connection_string;
    PGconn* db = PQconnectdb(connection_string.c_str());
    if (!db || PQstatus(db) != CONNECTION_OK) {
        if (db) last_error_ = PQerrorMessage(db);
        if (db) {
            PQfinish(db);
        }
        return false;
    }
    connection_ = db;
    connected_ = true;
    last_error_.clear();
    return true;
}

void PostgreSQLDatabase::disconnect() {
    if (connection_) {
        PQfinish(connection(connection_));
        connection_ = nullptr;
    }
    connected_ = false;
}

ResultSet PostgreSQLDatabase::execute(const std::string& query) {
    if (!connected_ || !connection_) {
        throw std::runtime_error("Database not connected");
    }
    return read_result(connection(connection_), PQexec(connection(connection_), query.c_str()));
}

ResultSet PostgreSQLDatabase::execute(
    const std::string& query, const std::vector<std::string>& params) {
    if (!connected_ || !connection_) {
        throw std::runtime_error("Database not connected");
    }
    std::vector<const char*> values;
    values.reserve(params.size());
    for (const auto& param : params) {
        values.push_back(param.c_str());
    }
    PGconn* db = connection(connection_);
    PGresult* result = PQexecParams(
        db, numbered_placeholders(query).c_str(),
        static_cast<int>(values.size()), nullptr, values.data(), nullptr, nullptr, 0);
    return read_result(db, result);
}

bool PostgreSQLDatabase::begin_transaction() {
    try { execute("BEGIN"); return true; } catch (const std::exception&) { return false; }
}

bool PostgreSQLDatabase::commit() {
    try { execute("COMMIT"); return true; } catch (const std::exception&) { return false; }
}

bool PostgreSQLDatabase::rollback() {
    try { execute("ROLLBACK"); return true; } catch (const std::exception&) { return false; }
}

bool PostgreSQLDatabase::create_table_from_model(const Model& model) {
    try {
        execute(CRUDGenerator(model, db_provider_).generate_create_table());
        return true;
    } catch (const std::exception& error) { last_error_ = error.what(); return false; }
}

bool PostgreSQLDatabase::drop_table(const std::string& table_name) {
    try {
        execute("DROP TABLE IF EXISTS " + quote_identifier(table_name, db_provider_));
        return true;
    } catch (const std::exception& error) { last_error_ = error.what(); return false; }
}

} // namespace sql
} // namespace ml
