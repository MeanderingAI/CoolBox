#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace sun::data {

struct SqlResult {
    bool ok = true;
    std::string error;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int affected_rows = 0;
    long long last_insert_id = 0;
};

class RelationalDbEngine {
public:
    explicit RelationalDbEngine(std::string storage_hint = "");

    SqlResult execute(const std::string& sql);

private:
    struct Table {
        std::vector<std::string> columns;
        std::unordered_map<std::string, std::size_t> column_index;
        std::vector<std::vector<std::string>> rows;
        long long auto_increment = 1;
    };

    std::string storage_hint_;
    bool in_tx_ = false;
    std::unordered_map<std::string, Table> tables_;
    std::unordered_map<std::string, Table> tx_snapshot_;

    SqlResult handle_create_table(const std::string& sql_upper, const std::string& sql_raw);
    SqlResult handle_insert(const std::string& sql_upper, const std::string& sql_raw);
    SqlResult handle_select(const std::string& sql_upper, const std::string& sql_raw) const;

    static std::string trim(const std::string& input);
    static std::string to_upper(std::string input);
    static std::vector<std::string> split_csv(const std::string& input);
    static std::vector<std::string> parse_parenthesized_csv(const std::string& sql_raw, std::size_t open_paren_pos);
    static std::string strip_quotes(const std::string& value);

    static std::string keyword_after(const std::string& sql_upper, const std::string& keyword);
    static std::size_t find_keyword(const std::string& sql_upper, const std::string& keyword);
};

}  // namespace sun::data
