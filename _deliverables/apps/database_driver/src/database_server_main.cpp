#include "cli_tools.hpp"
#include "database.h"
#include "sun_pooled_sql_server.hpp"
#include "sun_relational_db.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace {

std::string format_sun_result(const sun::data::SqlResult& result) {
    std::ostringstream out;

    if (!result.columns.empty()) {
        out << "columns:";
        for (const auto& col : result.columns) {
            out << ' ' << col;
        }
        out << '\n';
    }

    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out << " | ";
            }
            const std::string col_name = i < result.columns.size() ? result.columns[i] : ("c" + std::to_string(i));
            out << col_name << '=' << row[i];
        }
        out << '\n';
    }

    out << "affected_rows=" << result.affected_rows
        << " last_insert_id=" << result.last_insert_id;

    return out.str();
}

std::string format_sqlite_result(const ml::sql::ResultSet& result) {
    std::ostringstream out;
    if (!result.columns.empty()) {
        out << "columns:";
        for (const auto& col : result.columns) {
            out << ' ' << col;
        }
        out << '\n';
    }

    for (const auto& row : result.rows) {
        bool first = true;
        for (const auto& col : result.columns) {
            if (!first) {
                out << " | ";
            }
            const auto it = row.find(col);
            out << col << '=' << (it == row.end() ? "" : it->second);
            first = false;
        }
        out << '\n';
    }

    out << "affected_rows=" << result.affected_rows
        << " last_insert_id=" << result.last_insert_id;

    return out.str();
}

}  // namespace

int main(int argc, const char* const argv[]) {
    using namespace os_generics::cli;

    CommandLineParser parser;
    parser.set_program_name("database_server");
    parser.set_description("Pooled SQL server for database_client remote connections.");
    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"provider", 'p', true, false, "PROVIDER", "Database provider for sqlite mode (default: sqlite)."});
    parser.add_option({"db", 'd', true, false, "PATH", "Database path or storage hint (default: database_driver.sqlite3)."});
    parser.add_option({"engine", '\0', true, false, "internal|sqlite", "Execution engine (default: internal)."});
    parser.add_option({"host", '\0', true, false, "HOST", "Bind host (default: 127.0.0.1)."});
    parser.add_option({"port", '\0', true, false, "PORT", "Bind TCP/UDP port (default: 55432)."});
    parser.add_option({"workers", '\0', true, false, "N", "Worker thread count for network pooling (default: 4)."});
    parser.add_option({"transport", '\0', true, false, "tcp|udp|both", "Enabled transport(s) (default: both)."});

    const auto parsed = parser.parse_argv(argc, argv);
    if (!parsed.ok()) {
        for (const auto& err : parsed.errors) {
            std::cerr << "Error: " << err << "\n";
        }
        std::cerr << "\n" << parser.render_help() << "\n";
        return 1;
    }

    if (parsed.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        return 0;
    }

    const std::string engine = parsed.option_value("engine", "internal");
    const std::string provider = parsed.option_value("provider", "sqlite");
    const std::string db_path = parsed.option_value("db", "database_driver.sqlite3");
    const std::string host = parsed.option_value("host", "127.0.0.1");
    const int port = std::stoi(parsed.option_value("port", "55432"));
    const std::size_t workers = static_cast<std::size_t>(std::max(1, std::stoi(parsed.option_value("workers", "4"))));
    const std::string transport = parsed.option_value("transport", "both");

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port: " << port << "\n";
        return 1;
    }

    sun::comms::network::ServerConfig cfg;
    cfg.host = host;
    cfg.port = port;
    cfg.worker_threads = workers;
    cfg.enable_tcp = (transport == "both" || transport == "tcp");
    cfg.enable_udp = (transport == "both" || transport == "udp");

    if (!cfg.enable_tcp && !cfg.enable_udp) {
        std::cerr << "transport must be one of tcp|udp|both\n";
        return 1;
    }

    std::mutex engine_mutex;

    if (engine == "internal") {
        sun::data::RelationalDbEngine internal_engine(db_path);

        sun::comms::network::PooledSqlServer server(
            cfg,
            [&](const std::string& query) {
                std::lock_guard<std::mutex> lock(engine_mutex);
                const auto result = internal_engine.execute(query);
                if (!result.ok) {
                    return sun::comms::network::QueryResponse{false, result.error};
                }
                return sun::comms::network::QueryResponse{true, format_sun_result(result)};
            });

        std::cout << "database_server listening on " << host << ':' << port
                  << " engine=internal db_hint=" << db_path
                  << " transport=" << transport << " workers=" << workers << "\n";
        if (!server.run()) {
            std::cerr << "Failed to start pooled server\n";
            return 1;
        }
        return 0;
    }

    try {
        std::unique_ptr<ml::sql::Database> db = ml::sql::Database::create(provider);
        if (!db->connect(db_path)) {
            std::cerr << "Failed to connect to database: " << db_path << "\n";
            return 1;
        }

        sun::comms::network::PooledSqlServer server(
            cfg,
            [&](const std::string& query) {
                std::lock_guard<std::mutex> lock(engine_mutex);
                try {
                    const auto result = db->execute(query);
                    return sun::comms::network::QueryResponse{true, format_sqlite_result(result)};
                } catch (const std::exception& ex) {
                    return sun::comms::network::QueryResponse{false, ex.what()};
                }
            });

        std::cout << "database_server listening on " << host << ':' << port
                  << " engine=sqlite provider=" << provider << " db=" << db_path
                  << " transport=" << transport << " workers=" << workers << "\n";
        if (!server.run()) {
            std::cerr << "Failed to start pooled server\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "database_server error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
