#include "sql_tokenizer.h"
#include "btree.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" const char* get_metadata_management_library_name();
extern "C" const char* get_metadata_management_library_version();
extern "C" const char* get_metadata_management_library_description();

namespace {

using ml::sql::parser::SQLTokenizer;
using ml::sql::parser::Token;
using ml::sql::parser::TokenType;
using data_structures::BPlusTree;

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

std::string trim(const std::string& input) {
    std::size_t begin = 0;
    while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin]))) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(begin, end - begin);
}

std::string join_csv(const std::vector<std::string>& fields) {
    std::ostringstream buffer;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            buffer << ",";
        }
        buffer << fields[i];
    }
    return buffer.str();
}

std::vector<std::string> split_csv(const std::string& row) {
    std::vector<std::string> fields;
    std::string current;
    std::istringstream stream(row);
    while (std::getline(stream, current, ',')) {
        fields.push_back(current);
    }
    if (!row.empty() && row.back() == ',') {
        fields.push_back("");
    }
    return fields;
}

struct Table {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

class BasicDatabase {
public:
    explicit BasicDatabase(std::filesystem::path root)
        : tokenizer_(), root_path_(std::move(root)) {
        std::filesystem::create_directories(root_path_);
    }

    std::string execute(const std::string& sql) {
        const auto tokens = tokenizer_.tokenize_stripped(sql);
        if (tokens.empty() || tokens.front().type == TokenType::END_OF_INPUT) {
            return "Empty statement";
        }

        const std::string head = upper_token(tokens, 0);
        if (head == "CREATE") {
            return handle_create(tokens);
        }
        if (head == "INSERT") {
            return handle_insert(tokens);
        }
        if (head == "SELECT") {
            return handle_select(tokens);
        }
        if (head == "SHOW") {
            return handle_show(tokens);
        }

        return "Unsupported command. Try CREATE TABLE, INSERT INTO, SELECT, or SHOW TABLES.";
    }

private:
    std::string upper_token(const std::vector<Token>& tokens, std::size_t index) const {
        if (index >= tokens.size() || tokens[index].type == TokenType::END_OF_INPUT) {
            throw std::runtime_error("Unexpected end of statement");
        }
        return to_upper(tokens[index].value);
    }

    std::string raw_token(const std::vector<Token>& tokens, std::size_t index) const {
        if (index >= tokens.size() || tokens[index].type == TokenType::END_OF_INPUT) {
            throw std::runtime_error("Unexpected end of statement");
        }
        return tokens[index].value;
    }

    std::filesystem::path table_file_path(const std::string& table_name) const {
        return root_path_ / (table_name + ".tbl");
    }

    bool table_exists_in_storage(const std::string& table_name) const {
        return std::filesystem::exists(table_file_path(table_name));
    }

    void persist_table(const std::string& table_name) {
        const auto it = tables_.find(table_name);
        if (it == tables_.end()) {
            throw std::runtime_error("Cannot persist missing table: " + table_name);
        }

        std::ofstream out(table_file_path(table_name), std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Unable to write table file: " + table_name);
        }

        out << join_csv(it->second.columns) << "\n";
        for (const auto& row : it->second.rows) {
            out << join_csv(row) << "\n";
        }
    }

    Table load_table_from_storage(const std::string& table_name) const {
        std::ifstream in(table_file_path(table_name), std::ios::binary);
        if (!in) {
            throw std::runtime_error("Table does not exist: " + table_name);
        }

        std::string line;
        Table table;

        if (!std::getline(in, line)) {
            throw std::runtime_error("Table file is empty: " + table_name);
        }

        table.columns = split_csv(line);
        while (std::getline(in, line)) {
            if (!line.empty()) {
                table.rows.push_back(split_csv(line));
            }
        }

        return table;
    }

    Table& ensure_table_loaded(const std::string& table_name) {
        const auto loaded = tables_.find(table_name);
        if (loaded != tables_.end()) {
            return loaded->second;
        }

        if (!table_exists_in_storage(table_name)) {
            throw std::runtime_error("Table does not exist: " + table_name);
        }

        auto inserted = tables_.emplace(table_name, load_table_from_storage(table_name));
        return inserted.first->second;
    }

    std::vector<std::string> parse_parenthesized_list(const std::vector<Token>& tokens, std::size_t start_index) const {
        if (start_index >= tokens.size() || tokens[start_index].type != TokenType::OPEN_PAREN) {
            throw std::runtime_error("Expected '('");
        }

        std::vector<std::string> values;
        std::size_t i = start_index + 1;
        for (; i < tokens.size(); ++i) {
            const auto type = tokens[i].type;
            if (type == TokenType::END_OF_INPUT) {
                break;
            }
            if (type == TokenType::CLOSE_PAREN) {
                break;
            }
            if (type == TokenType::COMMA) {
                continue;
            }

            values.push_back(tokens[i].value);
        }

        if (i >= tokens.size() || tokens[i].type != TokenType::CLOSE_PAREN) {
            throw std::runtime_error("Expected ')' to close list");
        }

        return values;
    }

    std::string handle_create(const std::vector<Token>& tokens) {
        if (upper_token(tokens, 1) != "TABLE") {
            throw std::runtime_error("Expected TABLE after CREATE");
        }

        const std::string table_name = raw_token(tokens, 2);
        if (tables_.count(table_name) > 0 || table_exists_in_storage(table_name)) {
            throw std::runtime_error("Table already exists: " + table_name);
        }

        const std::vector<std::string> columns = parse_parenthesized_list(tokens, 3);
        if (columns.empty()) {
            throw std::runtime_error("CREATE TABLE requires at least one column");
        }

        Table table;
        table.columns = columns;
        table.rows.clear();
        tables_.emplace(table_name, std::move(table));
        
        // Create B+ tree for this table
        btree_storage_.emplace(table_name, std::make_shared<BPlusTree<int, std::string>>());
        persist_table(table_name);

        return "Table created: " + table_name;
    }

    std::string handle_insert(const std::vector<Token>& tokens) {
        if (upper_token(tokens, 1) != "INTO") {
            throw std::runtime_error("Expected INTO after INSERT");
        }

        const std::string table_name = raw_token(tokens, 2);
        Table& table = ensure_table_loaded(table_name);

        std::size_t values_index = 3;
        while (values_index < tokens.size() && tokens[values_index].type != TokenType::END_OF_INPUT) {
            if (to_upper(tokens[values_index].value) == "VALUES") {
                break;
            }
            ++values_index;
        }
        if (values_index >= tokens.size() || tokens[values_index].type == TokenType::END_OF_INPUT) {
            throw std::runtime_error("Expected VALUES clause in INSERT statement");
        }

        const std::vector<std::string> row = parse_parenthesized_list(tokens, values_index + 1);
        if (row.size() != table.columns.size()) {
            throw std::runtime_error("INSERT value count does not match table column count");
        }

        // Insert into in-memory table for backward compatibility
        table.rows.push_back(row);
        
        // Also insert into B+ tree using row index as key
        int row_key = static_cast<int>(table.rows.size() - 1);
        std::string row_csv = join_csv(row);
        
        if (btree_storage_.find(table_name) == btree_storage_.end()) {
            btree_storage_[table_name] = std::make_shared<BPlusTree<int, std::string>>();
        }
        btree_storage_[table_name]->insert(row_key, row_csv);
        
        persist_table(table_name);
        return "1 row inserted into " + table_name;
    }

    std::string handle_select(const std::vector<Token>& tokens) {
        if (tokens.size() < 5 || tokens[1].type != TokenType::STAR || upper_token(tokens, 2) != "FROM") {
            throw std::runtime_error("Only SELECT * FROM <table> is supported");
        }

        const std::string table_name = raw_token(tokens, 3);
        Table& table = ensure_table_loaded(table_name);

        std::ostringstream out;
        out << "Table: " << table_name << "\n";
        out << join_csv(table.columns) << "\n";
        
        // Display rows either from in-memory storage or B+ tree
        if (btree_storage_.find(table_name) != btree_storage_.end()) {
            auto entries = btree_storage_[table_name]->all_entries();
            for (const auto& entry : entries) {
                out << entry.second << "\n";
            }
            out << "Rows: " << entries.size();
        } else {
            for (const auto& row : table.rows) {
                out << join_csv(row) << "\n";
            }
            out << "Rows: " << table.rows.size();
        }
        return out.str();
    }

    std::string handle_show(const std::vector<Token>& tokens) {
        if (upper_token(tokens, 1) != "TABLES") {
            throw std::runtime_error("Only SHOW TABLES is supported");
        }

        for (const auto& item : std::filesystem::directory_iterator(root_path_)) {
            if (!item.is_regular_file()) {
                continue;
            }
            const auto path = item.path();
            if (path.extension() != ".tbl") {
                continue;
            }

            const std::string table_name = path.stem().string();
            if (tables_.count(table_name) == 0) {
                tables_.emplace(table_name, load_table_from_storage(table_name));
                btree_storage_.emplace(table_name, std::make_shared<BPlusTree<int, std::string>>());
            }
        }

        if (tables_.empty()) {
            return "No tables found.";
        }

        std::ostringstream out;
        out << "Tables:";
        for (const auto& entry : tables_) {
            out << "\n- " << entry.first;
        }
        return out.str();
    }

private:
    SQLTokenizer tokenizer_;
    std::filesystem::path root_path_;
    std::unordered_map<std::string, Table> tables_;
    std::unordered_map<std::string, std::shared_ptr<BPlusTree<int, std::string>>> btree_storage_;
};

} // namespace

int main() {
    std::cout << "database_app: basic SQL shell\n";
    std::cout << "using Trekker library: " << get_metadata_management_library_name();
    std::cout << " v" << get_metadata_management_library_version() << "\n";
    std::cout << get_metadata_management_library_description() << "\n\n";

    std::cout << "Supported commands:\n";
    std::cout << "- CREATE TABLE users (id, name);\n";
    std::cout << "- INSERT INTO users VALUES (1, 'alice');\n";
    std::cout << "- SELECT * FROM users;\n";
    std::cout << "- SHOW TABLES;\n";
    std::cout << "- EXIT\n\n";

    BasicDatabase database(std::filesystem::path("database_data"));

    std::string line;
    while (true) {
        std::cout << "db> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const std::string upper = to_upper(line);
        if (upper == "EXIT" || upper == "QUIT") {
            break;
        }

        try {
            std::cout << database.execute(line) << "\n";
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
    }

    return 0;
}
