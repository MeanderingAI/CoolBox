#ifndef BAYESIAN_NETWORK_H
#define BAYESIAN_NETWORK_H
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
// Matrix support
#include "../../../backages/DATASTRUCTURE/matrix/headers/matrix_dense.h"

namespace bayesian_db {

// Represents a node (table) in the Bayesian Network DB
struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::vector<std::unordered_map<std::string, std::string>> rows;
    // Mapping from string to int for each column
    std::unordered_map<std::string, std::unordered_map<std::string, int>> encoders;
    std::unordered_map<std::string, std::vector<std::string>> decoders;
};

class BayesianNetworkDB {
public:
    void add_table(const std::string& name, const std::vector<std::string>& columns);
    void insert(const std::string& table, const std::unordered_map<std::string, std::string>& row);
    std::vector<std::unordered_map<std::string, std::string>> query(const std::string& table, const std::string& column, const std::string& value) const;
    std::vector<std::string> list_tables() const;
    std::optional<std::unordered_map<std::string, std::string>> map(const std::string& table, const std::unordered_map<std::string, std::string>& evidence) const;
    std::optional<std::unordered_map<std::string, std::string>> mle(const std::string& table, const std::unordered_map<std::string, std::string>& evidence) const;
    // Encode a table as a matrix (strings to ints)
    std::unique_ptr<matrix::DenseMatrix> table_to_matrix(const std::string& table) const;
    // Decode a matrix row to string values
    std::unordered_map<std::string, std::string> decode_row(const std::string& table, const std::vector<int>& encoded_row) const;
private:
    std::unordered_map<std::string, Table> tables_;
};

} // namespace bayesian_db

#endif // BAYESIAN_NETWORK_H

