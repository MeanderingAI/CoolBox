#include "cli_tools.hpp"
#include "database.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

void print_result(const ml::sql::ResultSet& result) {
    if (!result.columns.empty()) {
        std::cout << "columns:";
        for (const auto& col : result.columns) {
            std::cout << ' ' << col;
        }
        std::cout << "\n";
    }

    for (const auto& row : result.rows) {
        bool first = true;
        for (const auto& col : result.columns) {
            if (!first) {
                std::cout << " | ";
            }
            const auto it = row.find(col);
            std::cout << col << '=' << (it == row.end() ? "" : it->second);
            first = false;
        }
        std::cout << "\n";
    }

    std::cout << "affected_rows=" << result.affected_rows
              << " last_insert_id=" << result.last_insert_id << "\n";
}

}  // namespace

int main(int argc, const char* const argv[]) {
    using namespace os_generics::cli;

    CommandLineParser parser;
    parser.set_program_name("database_driver");
    parser.set_description("Execute SQL statements against a local database file.");
    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"provider", 'p', true, false, "PROVIDER", "Database provider (default: sqlite)."});
    parser.add_option({"db", 'd', true, false, "PATH", "Database path (default: database_driver.sqlite3)."});
    parser.add_option({"query", 'q', true, false, "SQL", "SQL statement to execute."});

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

    const std::string provider = parsed.option_value("provider", "sqlite");
    const std::string db_path = parsed.option_value("db", "database_driver.sqlite3");
    const std::string query = parsed.option_value("query", "");

    if (query.empty()) {
        std::cerr << "Error: --query is required unless --help is used.\n\n"
                  << parser.render_help() << "\n";
        return 1;
    }

    try {
        std::unique_ptr<ml::sql::Database> db = ml::sql::Database::create(provider);
        if (!db->connect(db_path)) {
            std::cerr << "Failed to connect to database: " << db_path << "\n";
            return 1;
        }

        const auto result = db->execute(query);
        print_result(result);
        db->disconnect();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "database_driver error: " << ex.what() << "\n";
        return 1;
    }
}
