#include "../headers/bayesian_network.h"
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>
#include <stdexcept>

using matrix::DenseMatrix;

namespace bayesian_db {

// Helper: encode a string value for a column, updating encoder/decoder if needed
static int encode_value(Table& table, const std::string& column, const std::string& value) {
    auto& encoder = table.encoders[column];
    auto& decoder = table.decoders[column];
    auto it = encoder.find(value);
    if (it != encoder.end()) return it->second;
    int code = static_cast<int>(encoder.size());
    encoder[value] = code;
    decoder.push_back(value);
    return code;
}

// Helper: decode an int value for a column
static std::string decode_value(const Table& table, const std::string& column, int code) {
    const auto& decoder = table.decoders.at(column);
    if (code < 0 || static_cast<size_t>(code) >= decoder.size()) return "";
    return decoder[code];
}

void BayesianNetworkDB::add_table(const std::string& name, const std::vector<std::string>& columns) {
    tables_[name] = Table{name, columns, {}, {}, {}};
}

void BayesianNetworkDB::insert(const std::string& table, const std::unordered_map<std::string, std::string>& row) {
    if (tables_.count(table)) {
        auto& tbl = tables_[table];
        // Update encoders/decoders
        for (const auto& col : tbl.columns) {
            auto it = row.find(col);
            if (it != row.end()) encode_value(tbl, col, it->second);
        }
        tbl.rows.push_back(row);
    }
}

std::vector<std::unordered_map<std::string, std::string>> BayesianNetworkDB::query(const std::string& table, const std::string& column, const std::string& value) const {
    std::vector<std::unordered_map<std::string, std::string>> result;
    auto it = tables_.find(table);
    if (it != tables_.end()) {
        for (const auto& row : it->second.rows) {
            auto col_it = row.find(column);
            if (col_it != row.end() && col_it->second == value) {
                result.push_back(row);
            }
        }
    }
    return result;
}

std::vector<std::string> BayesianNetworkDB::list_tables() const {
    std::vector<std::string> names;
    for (const auto& kv : tables_) names.push_back(kv.first);
    return names;
}

std::optional<std::unordered_map<std::string, std::string>> BayesianNetworkDB::map(const std::string& table, const std::unordered_map<std::string, std::string>& evidence) const {
    auto it = tables_.find(table);
    if (it == tables_.end()) return std::nullopt;
    const auto& rows = it->second.rows;
    auto row_key = [](const std::unordered_map<std::string, std::string>& r) {
        std::map<std::string, std::string> sorted(r.begin(), r.end());
        std::string key;
        for (const auto& [k, v] : sorted) key += k + '=' + v + ';';
        return key;
    };
    std::unordered_map<std::string, int> counts;
    int max_count = 0;
    std::optional<std::unordered_map<std::string, std::string>> result;
    for (const auto& row : rows) {
        bool match = true;
        for (const auto& [col, val] : evidence) {
            auto rit = row.find(col);
            if (rit == row.end() || rit->second != val) {
                match = false;
                break;
            }
        }
        if (match) {
            auto k = row_key(row);
            ++counts[k];
            if (counts[k] > max_count) {
                max_count = counts[k];
                result = row;
            }
        }
    }
    return result;
}

std::optional<std::unordered_map<std::string, std::string>> BayesianNetworkDB::mle(const std::string& table, const std::unordered_map<std::string, std::string>& evidence) const {
    return map(table, evidence);
}

std::unique_ptr<DenseMatrix> BayesianNetworkDB::table_to_matrix(const std::string& table) const {
    auto it = tables_.find(table);
    if (it == tables_.end()) throw std::runtime_error("Table not found");
    const auto& tbl = it->second;
    size_t nrows = tbl.rows.size();
    size_t ncols = tbl.columns.size();
    std::vector<double> data(nrows * ncols, 0.0);
    for (size_t i = 0; i < nrows; ++i) {
        const auto& row = tbl.rows[i];
        for (size_t j = 0; j < ncols; ++j) {
            const std::string& col = tbl.columns[j];
            auto itv = row.find(col);
            if (itv != row.end()) {
                auto itenc = tbl.encoders.find(col);
                if (itenc != tbl.encoders.end()) {
                    auto itcode = itenc->second.find(itv->second);
                    if (itcode != itenc->second.end()) {
                        data[i * ncols + j] = static_cast<double>(itcode->second);
                    }
                }
            }
        }
    }
    return std::make_unique<DenseMatrix>(data, nrows, ncols);
}

std::unordered_map<std::string, std::string> BayesianNetworkDB::decode_row(const std::string& table, const std::vector<int>& encoded_row) const {
    std::unordered_map<std::string, std::string> result;
    auto it = tables_.find(table);
    if (it == tables_.end()) return result;
    const auto& tbl = it->second;
    size_t ncols = tbl.columns.size();
    for (size_t j = 0; j < ncols && j < encoded_row.size(); ++j) {
        const std::string& col = tbl.columns[j];
        result[col] = decode_value(tbl, col, encoded_row[j]);
    }
    return result;
}

} // namespace bayesian_db
