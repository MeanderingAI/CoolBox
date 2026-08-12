#include "endgame_table_compression.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {

void validate_bits(const std::vector<int>& bits, std::size_t bit_count) {
    if (bits.size() != bit_count) throw std::invalid_argument("Endgame table bit-vector length mismatch");
    for (int bit : bits) {
        if (bit != 0 && bit != 1) throw std::invalid_argument("Endgame table bits must be 0 or 1");
    }
}

int common_outcome(const std::vector<ml::endgame::EndgameEntry>& entries) {
    if (entries.empty()) return ml::endgame::unknown_outcome;
    const int outcome = entries.front().outcome;
    for (const auto& entry : entries) {
        if (entry.outcome != outcome) return ml::endgame::unknown_outcome;
    }
    return outcome;
}

double entropy(const std::vector<ml::endgame::EndgameEntry>& entries) {
    if (entries.empty()) return 0.0;
    std::map<int, int> counts;
    for (const auto& entry : entries) ++counts[entry.outcome];
    double value = 0.0;
    for (const auto& item : counts) {
        const double p = static_cast<double>(item.second) / static_cast<double>(entries.size());
        value -= p * std::log2(p);
    }
    return value;
}

std::string node_key(int variable, int outcome, int zero, int one) {
    return std::to_string(variable) + ":" + std::to_string(outcome) + ":" + std::to_string(zero) + ":" + std::to_string(one);
}

ml::endgame::Cube supercube(const std::vector<ml::endgame::Cube>& cubes) {
    ml::endgame::Cube result = cubes.front();
    for (const auto& cube : cubes) {
        for (std::size_t index = 0; index < result.values.size(); ++index) {
            if (result.values[index] != cube.values[index]) result.values[index] = -1;
        }
    }
    return result;
}

} // namespace

namespace ml {
namespace endgame {

bool Cube::contains(const std::vector<int>& bits) const {
    if (bits.size() != values.size()) return false;
    for (std::size_t index = 0; index < bits.size(); ++index) {
        if (values[index] != -1 && values[index] != bits[index]) return false;
    }
    return true;
}

void DecisionDagTable::build(const std::vector<EndgameEntry>& entries, std::size_t bit_count) {
    for (const auto& entry : entries) validate_bits(entry.bits, bit_count);
    nodes_.clear();
    std::vector<int> variables(bit_count);
    std::iota(variables.begin(), variables.end(), 0);
    root_ = build_recursive(entries, variables);
}

int DecisionDagTable::query(const std::vector<int>& bits) const {
    if (root_ < 0) return unknown_outcome;
    int node_index = root_;
    while (node_index >= 0) {
        const Node& node = nodes_[static_cast<std::size_t>(node_index)];
        if (node.variable < 0) return node.outcome;
        node_index = bits[static_cast<std::size_t>(node.variable)] == 0 ? node.zero : node.one;
    }
    return unknown_outcome;
}

std::size_t DecisionDagTable::node_count() const { return nodes_.size(); }

int DecisionDagTable::build_recursive(const std::vector<EndgameEntry>& entries, const std::vector<int>& variables) {
    const int outcome = common_outcome(entries);
    if (outcome != unknown_outcome || variables.empty() || entries.empty()) {
        return intern_node({-1, outcome, -1, -1});
    }

    const double base_entropy = entropy(entries);
    double best_gain = -std::numeric_limits<double>::infinity();
    int best_variable = variables.front();
    for (int variable : variables) {
        std::vector<EndgameEntry> zero_entries;
        std::vector<EndgameEntry> one_entries;
        for (const auto& entry : entries) {
            (entry.bits[static_cast<std::size_t>(variable)] == 0 ? zero_entries : one_entries).push_back(entry);
        }
        const double weighted_entropy = (static_cast<double>(zero_entries.size()) * entropy(zero_entries)
            + static_cast<double>(one_entries.size()) * entropy(one_entries)) / static_cast<double>(entries.size());
        const double gain = base_entropy - weighted_entropy;
        if (gain > best_gain) {
            best_gain = gain;
            best_variable = variable;
        }
    }

    std::vector<int> next_variables;
    for (int variable : variables) if (variable != best_variable) next_variables.push_back(variable);
    std::vector<EndgameEntry> zero_entries;
    std::vector<EndgameEntry> one_entries;
    for (const auto& entry : entries) {
        (entry.bits[static_cast<std::size_t>(best_variable)] == 0 ? zero_entries : one_entries).push_back(entry);
    }
    const int zero = build_recursive(zero_entries, next_variables);
    const int one = build_recursive(one_entries, next_variables);
    if (zero == one) return zero;
    return intern_node({best_variable, unknown_outcome, zero, one});
}

int DecisionDagTable::intern_node(const Node& node) {
    const std::string key = node_key(node.variable, node.outcome, node.zero, node.one);
    for (std::size_t index = 0; index < nodes_.size(); ++index) {
        if (node_key(nodes_[index].variable, nodes_[index].outcome, nodes_[index].zero, nodes_[index].one) == key) {
            return static_cast<int>(index);
        }
    }
    nodes_.push_back(node);
    return static_cast<int>(nodes_.size() - 1);
}

void MultiterminalDecisionDiagram::build(const std::vector<EndgameEntry>& entries, std::size_t bit_count, bool concretize_unknowns) {
    for (const auto& entry : entries) validate_bits(entry.bits, bit_count);
    nodes_.clear();
    root_ = build_ordered(entries, 0, bit_count, concretize_unknowns);
}

int MultiterminalDecisionDiagram::query(const std::vector<int>& bits) const {
    if (root_ < 0) return unknown_outcome;
    int node_index = root_;
    while (node_index >= 0) {
        const Node& node = nodes_[static_cast<std::size_t>(node_index)];
        if (node.variable < 0) return node.outcome;
        node_index = bits[static_cast<std::size_t>(node.variable)] == 0 ? node.zero : node.one;
    }
    return unknown_outcome;
}

std::size_t MultiterminalDecisionDiagram::node_count() const { return nodes_.size(); }

int MultiterminalDecisionDiagram::build_ordered(
    const std::vector<EndgameEntry>& entries,
    std::size_t variable,
    std::size_t bit_count,
    bool concretize_unknowns) {
    const int outcome = common_outcome(entries);
    if (outcome != unknown_outcome || variable == bit_count || entries.empty()) {
        return intern_node({-1, outcome, -1, -1});
    }
    std::vector<EndgameEntry> zero_entries;
    std::vector<EndgameEntry> one_entries;
    for (const auto& entry : entries) {
        (entry.bits[variable] == 0 ? zero_entries : one_entries).push_back(entry);
    }
    int zero = build_ordered(zero_entries, variable + 1, bit_count, concretize_unknowns);
    int one = build_ordered(one_entries, variable + 1, bit_count, concretize_unknowns);
    if (concretize_unknowns && nodes_[static_cast<std::size_t>(zero)].variable < 0 && nodes_[static_cast<std::size_t>(zero)].outcome == unknown_outcome) zero = one;
    if (concretize_unknowns && nodes_[static_cast<std::size_t>(one)].variable < 0 && nodes_[static_cast<std::size_t>(one)].outcome == unknown_outcome) one = zero;
    if (zero == one) return zero;
    return intern_node({static_cast<int>(variable), unknown_outcome, zero, one});
}

int MultiterminalDecisionDiagram::intern_node(const Node& node) {
    const std::string key = node_key(node.variable, node.outcome, node.zero, node.one);
    for (std::size_t index = 0; index < nodes_.size(); ++index) {
        if (node_key(nodes_[index].variable, nodes_[index].outcome, nodes_[index].zero, nodes_[index].one) == key) {
            return static_cast<int>(index);
        }
    }
    nodes_.push_back(node);
    return static_cast<int>(nodes_.size() - 1);
}

void LogicMinimizedTable::build(const std::vector<EndgameEntry>& entries, std::size_t bit_count, std::size_t merge_distance) {
    for (const auto& entry : entries) validate_bits(entry.bits, bit_count);
    bit_count_ = bit_count;
    baseline_ = entries;
    cubes_.clear();
    for (const auto& entry : entries) cubes_.push_back({entry.bits, entry.outcome});
    distance_merge(merge_distance);
    index_nodes_.clear();
    std::vector<std::size_t> cube_indices(cubes_.size());
    std::iota(cube_indices.begin(), cube_indices.end(), 0);
    std::vector<int> variables(bit_count_);
    std::iota(variables.begin(), variables.end(), 0);
    root_ = build_index(cube_indices, variables);
}

int LogicMinimizedTable::query(const std::vector<int>& bits) const {
    validate_bits(bits, bit_count_);
    return query_index(root_, bits);
}

const std::vector<Cube>& LogicMinimizedTable::cubes() const { return cubes_; }

void LogicMinimizedTable::distance_merge(std::size_t merge_distance) {
    if (merge_distance == 0 || cubes_.empty()) return;
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t left = 0; left < cubes_.size() && !changed; ++left) {
            for (std::size_t right = left + 1; right < cubes_.size() && !changed; ++right) {
                if (cubes_[left].outcome != cubes_[right].outcome) continue;
                std::size_t differing = 0;
                for (std::size_t bit = 0; bit < bit_count_; ++bit) {
                    if (cubes_[left].values[bit] != cubes_[right].values[bit]) ++differing;
                }
                if (differing == 0 || differing > merge_distance) continue;
                Cube candidate = supercube({cubes_[left], cubes_[right]});
                if (conflicts_with_baseline(candidate)) continue;
                cubes_[left] = candidate;
                cubes_.erase(cubes_.begin() + static_cast<long>(right));
                changed = true;
            }
        }
    }
}

bool LogicMinimizedTable::conflicts_with_baseline(const Cube& cube) const {
    for (const auto& entry : baseline_) {
        if (entry.outcome != cube.outcome && cube.contains(entry.bits)) return true;
    }
    return false;
}

int LogicMinimizedTable::build_index(const std::vector<std::size_t>& cube_indices, const std::vector<int>& variables) {
    if (cube_indices.empty()) return -1;
    if (cube_indices.size() <= 16 || variables.empty()) {
        index_nodes_.push_back({-1, -1, -1, -1, cube_indices});
        return static_cast<int>(index_nodes_.size() - 1);
    }

    long long best_score = std::numeric_limits<long long>::max();
    int best_variable = variables.front();
    for (int variable : variables) {
        long long zero_count = 0;
        long long one_count = 0;
        long long wildcard_count = 0;
        for (std::size_t cube_index : cube_indices) {
            const int value = cubes_[cube_index].values[static_cast<std::size_t>(variable)];
            if (value == 0) ++zero_count;
            else if (value == 1) ++one_count;
            else ++wildcard_count;
        }
        const long long score = zero_count * (zero_count + wildcard_count)
            + one_count * (one_count + wildcard_count)
            + 2 * wildcard_count * wildcard_count;
        if (score < best_score) {
            best_score = score;
            best_variable = variable;
        }
    }

    std::vector<std::size_t> zero_indices;
    std::vector<std::size_t> one_indices;
    std::vector<std::size_t> wildcard_indices;
    for (std::size_t cube_index : cube_indices) {
        const int value = cubes_[cube_index].values[static_cast<std::size_t>(best_variable)];
        if (value == 0) zero_indices.push_back(cube_index);
        else if (value == 1) one_indices.push_back(cube_index);
        else wildcard_indices.push_back(cube_index);
    }

    std::vector<int> next_variables;
    for (int variable : variables) if (variable != best_variable) next_variables.push_back(variable);

    const int node_index = static_cast<int>(index_nodes_.size());
    index_nodes_.push_back({best_variable, -1, -1, -1, {}});
    index_nodes_[static_cast<std::size_t>(node_index)].zero = build_index(zero_indices, next_variables);
    index_nodes_[static_cast<std::size_t>(node_index)].one = build_index(one_indices, next_variables);
    index_nodes_[static_cast<std::size_t>(node_index)].wildcard = build_index(wildcard_indices, next_variables);
    return node_index;
}

int LogicMinimizedTable::query_index(int node_index, const std::vector<int>& bits) const {
    if (node_index < 0) return unknown_outcome;
    const IndexNode& node = index_nodes_[static_cast<std::size_t>(node_index)];
    if (node.variable < 0) {
        for (std::size_t cube_index : node.cube_indices) {
            const Cube& cube = cubes_[cube_index];
            if (cube.contains(bits)) return cube.outcome;
        }
        return unknown_outcome;
    }

    const int bit = bits[static_cast<std::size_t>(node.variable)];
    const int exact_result = query_index(bit == 0 ? node.zero : node.one, bits);
    if (exact_result != unknown_outcome) return exact_result;
    return query_index(node.wildcard, bits);
}

} // namespace endgame
} // namespace ml