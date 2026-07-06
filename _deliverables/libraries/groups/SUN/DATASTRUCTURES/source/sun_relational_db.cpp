#include "sun_relational_db.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace sun::data {

RelationalDbEngine::RelationalDbEngine(std::string storage_hint)
    : storage_hint_(std::move(storage_hint)) {}

SqlResult RelationalDbEngine::execute(const std::string& sql) {
    const std::string raw = trim(sql);
    const std::string upper = to_upper(raw);

    if (upper.empty()) {
        return SqlResult{};
    }

    if (upper == "BEGIN" || upper == "BEGIN TRANSACTION") {
        if (in_tx_) {
            return SqlResult{false, "Transaction already active"};
        }
        tx_snapshot_ = tables_;
        in_tx_ = true;
        return SqlResult{};
    }

    if (upper == "COMMIT") {
        if (!in_tx_) {
            return SqlResult{false, "No active transaction"};
        }
        tx_snapshot_.clear();
        in_tx_ = false;
        return SqlResult{};
    }

    if (upper == "ROLLBACK") {
        if (!in_tx_) {
            return SqlResult{false, "No active transaction"};
        }
        tables_ = tx_snapshot_;
        tx_snapshot_.clear();
        in_tx_ = false;
        return SqlResult{};
    }

    if (upper.rfind("CREATE TABLE", 0) == 0) {
        return handle_create_table(upper, raw);
    }

    if (upper.rfind("INSERT INTO", 0) == 0) {
        return handle_insert(upper, raw);
    }

    if (upper.rfind("SELECT", 0) == 0) {
        return handle_select(upper, raw);
    }

    return SqlResult{false, "Unsupported SQL for internal engine"};
}

SqlResult RelationalDbEngine::handle_create_table(const std::string& sql_upper, const std::string& sql_raw) {
    std::size_t table_kw = find_keyword(sql_upper, "TABLE");
    if (table_kw == std::string::npos) {
        return SqlResult{false, "Malformed CREATE TABLE statement"};
    }

    std::size_t name_pos = table_kw + 5;
    while (name_pos < sql_upper.size() && std::isspace(static_cast<unsigned char>(sql_upper[name_pos]))) {
        ++name_pos;
    }

    if (sql_upper.compare(name_pos, 13, "IF NOT EXISTS") == 0) {
        name_pos += 13;
        while (name_pos < sql_upper.size() && std::isspace(static_cast<unsigned char>(sql_upper[name_pos]))) {
            ++name_pos;
        }
    }

    std::size_t name_end = name_pos;
    while (name_end < sql_upper.size() &&
           !std::isspace(static_cast<unsigned char>(sql_upper[name_end])) &&
           sql_upper[name_end] != '(') {
        ++name_end;
    }

    const std::string table_name = trim(sql_raw.substr(name_pos, name_end - name_pos));
    if (table_name.empty()) {
        return SqlResult{false, "Missing table name"};
    }

    if (tables_.find(table_name) != tables_.end()) {
        return SqlResult{};
    }

    std::size_t open = sql_raw.find('(', name_end);
    if (open == std::string::npos) {
        return SqlResult{false, "Missing column list"};
    }

    const std::vector<std::string> defs = parse_parenthesized_csv(sql_raw, open);
    if (defs.empty()) {
        return SqlResult{false, "Column list cannot be empty"};
    }

    Table table;
    for (const std::string& def : defs) {
        const std::string trimmed = trim(def);
        if (trimmed.empty()) {
            continue;
        }

        std::size_t split = 0;
        while (split < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[split]))) {
            ++split;
        }

        const std::string col = trim(trimmed.substr(0, split));
        if (col.empty()) {
            continue;
        }

        table.column_index[col] = table.columns.size();
        table.columns.push_back(col);
    }

    if (table.columns.empty()) {
        return SqlResult{false, "No valid columns parsed"};
    }

    tables_[table_name] = std::move(table);
    return SqlResult{};
}

SqlResult RelationalDbEngine::handle_insert(const std::string& sql_upper, const std::string& sql_raw) {
    std::size_t into_kw = find_keyword(sql_upper, "INTO");
    if (into_kw == std::string::npos) {
        return SqlResult{false, "Malformed INSERT statement"};
    }

    std::size_t table_start = into_kw + 4;
    while (table_start < sql_upper.size() && std::isspace(static_cast<unsigned char>(sql_upper[table_start]))) {
        ++table_start;
    }

    std::size_t table_end = table_start;
    while (table_end < sql_upper.size() &&
           !std::isspace(static_cast<unsigned char>(sql_upper[table_end])) &&
           sql_upper[table_end] != '(') {
        ++table_end;
    }

    const std::string table_name = trim(sql_raw.substr(table_start, table_end - table_start));
    auto table_it = tables_.find(table_name);
    if (table_it == tables_.end()) {
        return SqlResult{false, "Unknown table: " + table_name};
    }

    Table& table = table_it->second;

    std::size_t cols_open = sql_raw.find('(', table_end);
    std::size_t cols_close = std::string::npos;
    if (cols_open != std::string::npos) {
        cols_close = sql_raw.find(')', cols_open + 1);
    }

    std::vector<std::string> insert_columns;
    if (cols_open != std::string::npos && cols_close != std::string::npos) {
        insert_columns = parse_parenthesized_csv(sql_raw, cols_open);
    } else {
        insert_columns = table.columns;
    }

    std::size_t values_kw = find_keyword(sql_upper, "VALUES");
    if (values_kw == std::string::npos) {
        return SqlResult{false, "INSERT requires VALUES"};
    }

    std::size_t vals_open = sql_raw.find('(', values_kw);
    if (vals_open == std::string::npos) {
        return SqlResult{false, "VALUES list missing"};
    }

    std::vector<std::string> values = parse_parenthesized_csv(sql_raw, vals_open);
    if (insert_columns.size() != values.size()) {
        return SqlResult{false, "Column/value count mismatch"};
    }

    std::vector<std::string> row(table.columns.size(), "");
    for (std::size_t i = 0; i < insert_columns.size(); ++i) {
        const std::string col = trim(insert_columns[i]);
        auto col_it = table.column_index.find(col);
        if (col_it == table.column_index.end()) {
            return SqlResult{false, "Unknown column: " + col};
        }
        row[col_it->second] = strip_quotes(trim(values[i]));
    }

    auto id_col_it = table.column_index.find("id");
    if (id_col_it != table.column_index.end() && row[id_col_it->second].empty()) {
        row[id_col_it->second] = std::to_string(table.auto_increment++);
    }

    table.rows.push_back(std::move(row));

    SqlResult res;
    res.affected_rows = 1;
    res.last_insert_id = table.auto_increment - 1;
    return res;
}

SqlResult RelationalDbEngine::handle_select(const std::string& sql_upper, const std::string& sql_raw) const {
    std::size_t from_kw = find_keyword(sql_upper, "FROM");
    if (from_kw == std::string::npos) {
        return SqlResult{false, "Malformed SELECT statement"};
    }

    const std::string cols_part = trim(sql_raw.substr(6, from_kw - 6));

    std::size_t table_start = from_kw + 4;
    while (table_start < sql_upper.size() && std::isspace(static_cast<unsigned char>(sql_upper[table_start]))) {
        ++table_start;
    }

    std::size_t table_end = table_start;
    while (table_end < sql_upper.size() && !std::isspace(static_cast<unsigned char>(sql_upper[table_end])) && sql_upper[table_end] != ';') {
        ++table_end;
    }

    const std::string table_name = trim(sql_raw.substr(table_start, table_end - table_start));
    auto table_it = tables_.find(table_name);
    if (table_it == tables_.end()) {
        return SqlResult{false, "Unknown table: " + table_name};
    }

    const Table& table = table_it->second;

    std::vector<std::string> wanted_columns;
    if (to_upper(cols_part) == "*") {
        wanted_columns = table.columns;
    } else {
        wanted_columns = split_csv(cols_part);
        for (std::string& c : wanted_columns) {
            c = trim(c);
            if (table.column_index.find(c) == table.column_index.end()) {
                return SqlResult{false, "Unknown column in SELECT: " + c};
            }
        }
    }

    SqlResult res;
    res.columns = wanted_columns;
    for (const auto& src_row : table.rows) {
        std::vector<std::string> out_row;
        out_row.reserve(wanted_columns.size());
        for (const auto& col : wanted_columns) {
            out_row.push_back(src_row[table.column_index.at(col)]);
        }
        res.rows.push_back(std::move(out_row));
    }
    return res;
}

std::string RelationalDbEngine::trim(const std::string& input) {
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

std::string RelationalDbEngine::to_upper(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return input;
}

std::vector<std::string> RelationalDbEngine::split_csv(const std::string& input) {
    std::vector<std::string> out;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        out.push_back(item);
    }
    return out;
}

std::vector<std::string> RelationalDbEngine::parse_parenthesized_csv(const std::string& sql_raw, std::size_t open_paren_pos) {
    std::vector<std::string> out;

    int depth = 0;
    bool in_quote = false;
    std::string current;

    for (std::size_t i = open_paren_pos; i < sql_raw.size(); ++i) {
        const char ch = sql_raw[i];

        if (ch == '\'' && (i == 0 || sql_raw[i - 1] != '\\')) {
            in_quote = !in_quote;
            if (depth > 0) {
                current.push_back(ch);
            }
            continue;
        }

        if (!in_quote) {
            if (ch == '(') {
                ++depth;
                if (depth == 1) {
                    continue;
                }
            } else if (ch == ')') {
                --depth;
                if (depth == 0) {
                    out.push_back(current);
                    break;
                }
            } else if (ch == ',' && depth == 1) {
                out.push_back(current);
                current.clear();
                continue;
            }
        }

        if (depth >= 1) {
            current.push_back(ch);
        }
    }

    for (std::string& entry : out) {
        entry = trim(entry);
    }

    return out;
}

std::string RelationalDbEngine::strip_quotes(const std::string& value) {
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string RelationalDbEngine::keyword_after(const std::string& sql_upper, const std::string& keyword) {
    const std::size_t pos = find_keyword(sql_upper, keyword);
    if (pos == std::string::npos) {
        return "";
    }

    std::size_t start = pos + keyword.size();
    while (start < sql_upper.size() && std::isspace(static_cast<unsigned char>(sql_upper[start]))) {
        ++start;
    }

    std::size_t end = start;
    while (end < sql_upper.size() && !std::isspace(static_cast<unsigned char>(sql_upper[end])) && sql_upper[end] != ';') {
        ++end;
    }

    return sql_upper.substr(start, end - start);
}

std::size_t RelationalDbEngine::find_keyword(const std::string& sql_upper, const std::string& keyword) {
    return sql_upper.find(keyword);
}

}  // namespace sun::data
