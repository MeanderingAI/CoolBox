#include "../headers/database.h"
#include <mysql.h>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ml {
namespace sql {
namespace {

MYSQL* connection(void* handle) {
    return static_cast<MYSQL*>(handle);
}

struct MysqlConnectionOptions {
    std::string user;
    std::string password;
    std::string host = "localhost";
    std::string database;
    unsigned int port = 3306;
};

MysqlConnectionOptions parse_connection_uri(const std::string& uri) {
    constexpr const char* prefix = "mysql://";
    if (uri.compare(0, 8, prefix) != 0) {
        throw std::runtime_error(
            "MySQL connection string must use mysql://user:password@host:port/database");
    }
    const auto at = uri.find('@', 8);
    const auto slash = uri.find('/', at == std::string::npos ? 8 : at + 1);
    if (at == std::string::npos || slash == std::string::npos) {
        throw std::runtime_error(
            "MySQL connection string must use mysql://user:password@host:port/database");
    }

    MysqlConnectionOptions options;
    const auto colon = uri.find(':', 8);
    if (colon == std::string::npos || colon > at) {
        options.user = uri.substr(8, at - 8);
    } else {
        options.user = uri.substr(8, colon - 8);
        options.password = uri.substr(colon + 1, at - colon - 1);
    }

    const std::string host_port = uri.substr(at + 1, slash - at - 1);
    const auto port_separator = host_port.rfind(':');
    if (port_separator != std::string::npos) {
        options.host = host_port.substr(0, port_separator);
        const auto port = host_port.substr(port_separator + 1);
        try {
            const auto parsed = std::stoul(port);
            if (parsed == 0 || parsed > 65535) {
                throw std::runtime_error("MySQL port must be between 1 and 65535");
            }
            options.port = static_cast<unsigned int>(parsed);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("MySQL port is not a valid number");
        }
    } else if (!host_port.empty()) {
        options.host = host_port;
    }
    options.database = uri.substr(slash + 1);
    if (options.database.empty()) {
        throw std::runtime_error("MySQL connection string must include a database name");
    }
    return options;
}

ResultSet read_result(MYSQL* db, MYSQL_RES* mysql_result) {
    ResultSet result;
    if (mysql_result) {
        const unsigned int count = mysql_num_fields(mysql_result);
        MYSQL_FIELD* fields = mysql_fetch_fields(mysql_result);
        for (unsigned int i = 0; i < count; ++i) {
            result.columns.emplace_back(fields[i].name);
        }
        MYSQL_ROW values;
        while ((values = mysql_fetch_row(mysql_result))) {
            const unsigned long* lengths = mysql_fetch_lengths(mysql_result);
            Row row;
            for (unsigned int i = 0; i < count; ++i) {
                row[result.columns[i]] = values[i]
                    ? std::string(values[i], lengths[i]) : "";
            }
            result.rows.push_back(std::move(row));
        }
        mysql_free_result(mysql_result);
    } else if (mysql_field_count(db) != 0) {
        throw std::runtime_error(std::string("MySQL result retrieval failed: ") + mysql_error(db));
    }
    result.affected_rows = static_cast<int>(mysql_affected_rows(db));
    result.last_insert_id = static_cast<long long>(mysql_insert_id(db));
    return result;
}

ResultSet execute_prepared(MYSQL* db, const std::string& query,
                           const std::vector<std::string>& params) {
    MYSQL_STMT* statement = mysql_stmt_init(db);
    if (!statement) {
        throw std::runtime_error(std::string("MySQL statement initialization failed: ") +
                                 mysql_error(db));
    }
    const auto close_statement = [&statement]() {
        if (statement) {
            mysql_stmt_close(statement);
            statement = nullptr;
        }
    };
    if (mysql_stmt_prepare(statement, query.c_str(),
                          static_cast<unsigned long>(query.size())) != 0) {
        const std::string error = mysql_stmt_error(statement);
        close_statement();
        throw std::runtime_error("MySQL prepare failed: " + error);
    }

    std::vector<MYSQL_BIND> input(params.size());
    std::vector<unsigned long> input_lengths;
    input_lengths.reserve(params.size());
    for (const auto& param : params) {
        input_lengths.push_back(static_cast<unsigned long>(param.size()));
    }
    for (std::size_t i = 0; i < params.size(); ++i) {
        input[i] = MYSQL_BIND{};
        input[i].buffer_type = MYSQL_TYPE_STRING;
        input[i].buffer = const_cast<char*>(params[i].data());
        input[i].buffer_length = input_lengths[i];
        input[i].length = &input_lengths[i];
    }
    if (!input.empty() && mysql_stmt_bind_param(statement, input.data()) != 0) {
        const std::string error = mysql_stmt_error(statement);
        close_statement();
        throw std::runtime_error("MySQL parameter binding failed: " + error);
    }
    if (mysql_stmt_execute(statement) != 0) {
        const std::string error = mysql_stmt_error(statement);
        close_statement();
        throw std::runtime_error("MySQL execution failed: " + error);
    }

    ResultSet result;
    MYSQL_RES* metadata = mysql_stmt_result_metadata(statement);
    if (metadata) {
        const unsigned int count = mysql_num_fields(metadata);
        MYSQL_FIELD* fields = mysql_fetch_fields(metadata);
        for (unsigned int i = 0; i < count; ++i) {
            result.columns.emplace_back(fields[i].name);
        }
        std::vector<MYSQL_BIND> output(count);
        std::vector<unsigned long> lengths(count);
        using MysqlNullFlag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
        std::vector<MysqlNullFlag> nulls(count);
        std::vector<char> initial_buffers(count);
        for (unsigned int i = 0; i < count; ++i) {
            output[i] = MYSQL_BIND{};
            output[i].buffer_type = MYSQL_TYPE_STRING;
            output[i].buffer = &initial_buffers[i];
            output[i].buffer_length = 1;
            output[i].length = &lengths[i];
            output[i].is_null = &nulls[i];
        }
        if (mysql_stmt_bind_result(statement, output.data()) != 0) {
            const std::string error = mysql_stmt_error(statement);
            mysql_free_result(metadata);
            close_statement();
            throw std::runtime_error("MySQL result binding failed: " + error);
        }
        int fetch_status;
        while ((fetch_status = mysql_stmt_fetch(statement)) == 0 ||
               fetch_status == MYSQL_DATA_TRUNCATED) {
            Row row;
            for (unsigned int i = 0; i < count; ++i) {
                std::string value;
                if (!nulls[i]) {
                    std::vector<char> buffer(lengths[i] + 1);
                    MYSQL_BIND column = MYSQL_BIND{};
                    column.buffer_type = MYSQL_TYPE_STRING;
                    column.buffer = buffer.data();
                    column.buffer_length = static_cast<unsigned long>(buffer.size());
                    unsigned long fetched_length = 0;
                    column.length = &fetched_length;
                    column.is_null = &nulls[i];
                    if (mysql_stmt_fetch_column(statement, &column, i, 0) != 0) {
                        const std::string error = mysql_stmt_error(statement);
                        mysql_free_result(metadata);
                        close_statement();
                        throw std::runtime_error("MySQL result fetch failed: " + error);
                    }
                    value.assign(buffer.data(), fetched_length);
                }
                row[result.columns[i]] = std::move(value);
            }
            result.rows.push_back(std::move(row));
        }
        if (fetch_status != MYSQL_NO_DATA) {
            const std::string error = mysql_stmt_error(statement);
            mysql_free_result(metadata);
            close_statement();
            throw std::runtime_error("MySQL result fetch failed: " + error);
        }
        mysql_free_result(metadata);
    }
    result.affected_rows = static_cast<int>(mysql_stmt_affected_rows(statement));
    result.last_insert_id = static_cast<long long>(mysql_stmt_insert_id(statement));
    close_statement();
    return result;
}

} // namespace

MySQLDatabase::MySQLDatabase() : Database("mysql") {}

MySQLDatabase::~MySQLDatabase() {
    disconnect();
}

bool MySQLDatabase::connect(const std::string& connection_string) {
    disconnect();
    const auto options = parse_connection_uri(connection_string);
    MYSQL* db = mysql_init(nullptr);
    if (!db) {
        return false;
    }
    if (!mysql_real_connect(db, options.host.c_str(), options.user.c_str(),
                           options.password.c_str(), options.database.c_str(),
                           options.port, nullptr, 0)) {
        last_error_ = mysql_error(db);
        mysql_close(db);
        return false;
    }
    connection_string_ = connection_string;
    connection_ = db;
    connected_ = true;
    last_error_.clear();
    return true;
}

void MySQLDatabase::disconnect() {
    if (connection_) {
        mysql_close(connection(connection_));
        connection_ = nullptr;
    }
    connected_ = false;
}

ResultSet MySQLDatabase::execute(const std::string& query) {
    if (!connected_ || !connection_) {
        throw std::runtime_error("Database not connected");
    }
    MYSQL* db = connection(connection_);
    if (mysql_real_query(db, query.c_str(), static_cast<unsigned long>(query.size())) != 0) {
        throw std::runtime_error(std::string("MySQL query failed: ") + mysql_error(db));
    }
    return read_result(db, mysql_store_result(db));
}

ResultSet MySQLDatabase::execute(
    const std::string& query, const std::vector<std::string>& params) {
    if (!connected_ || !connection_) {
        throw std::runtime_error("Database not connected");
    }
    if (params.empty()) {
        return execute(query);
    }
    return execute_prepared(connection(connection_), query, params);
}

bool MySQLDatabase::begin_transaction() {
    try { execute("START TRANSACTION"); return true; } catch (const std::exception&) { return false; }
}

bool MySQLDatabase::commit() {
    try { execute("COMMIT"); return true; } catch (const std::exception&) { return false; }
}

bool MySQLDatabase::rollback() {
    try { execute("ROLLBACK"); return true; } catch (const std::exception&) { return false; }
}

bool MySQLDatabase::create_table_from_model(const Model& model) {
    try {
        execute(CRUDGenerator(model, db_provider_).generate_create_table());
        return true;
    } catch (const std::exception& error) { last_error_ = error.what(); return false; }
}

bool MySQLDatabase::drop_table(const std::string& table_name) {
    try {
        execute("DROP TABLE IF EXISTS " + quote_identifier(table_name, db_provider_));
        return true;
    } catch (const std::exception& error) { last_error_ = error.what(); return false; }
}

} // namespace sql
} // namespace ml
