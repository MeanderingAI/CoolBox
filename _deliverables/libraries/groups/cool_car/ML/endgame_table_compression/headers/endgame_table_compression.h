#ifndef ENDGAME_TABLE_COMPRESSION_H
#define ENDGAME_TABLE_COMPRESSION_H

#include <cstddef>
#include <vector>

namespace ml {
namespace endgame {

constexpr int unknown_outcome = -1;

struct EndgameEntry {
    std::vector<int> bits;
    int outcome = unknown_outcome;
};

struct Cube {
    std::vector<int> values;
    int outcome = unknown_outcome;

    bool contains(const std::vector<int>& bits) const;
};

class DecisionDagTable {
public:
    void build(const std::vector<EndgameEntry>& entries, std::size_t bit_count);
    int query(const std::vector<int>& bits) const;
    std::size_t node_count() const;

private:
    struct Node {
        int variable = -1;
        int outcome = unknown_outcome;
        int zero = -1;
        int one = -1;
    };

    std::vector<Node> nodes_;
    int root_ = -1;

    int build_recursive(const std::vector<EndgameEntry>& entries, const std::vector<int>& variables);
    int intern_node(const Node& node);
};

class MultiterminalDecisionDiagram {
public:
    void build(const std::vector<EndgameEntry>& entries, std::size_t bit_count, bool concretize_unknowns = true);
    int query(const std::vector<int>& bits) const;
    std::size_t node_count() const;

private:
    struct Node {
        int variable = -1;
        int outcome = unknown_outcome;
        int zero = -1;
        int one = -1;
    };

    std::vector<Node> nodes_;
    int root_ = -1;

    int build_ordered(const std::vector<EndgameEntry>& entries, std::size_t variable, std::size_t bit_count, bool concretize_unknowns);
    int intern_node(const Node& node);
};

class LogicMinimizedTable {
public:
    void build(const std::vector<EndgameEntry>& entries, std::size_t bit_count, std::size_t merge_distance = 1);
    int query(const std::vector<int>& bits) const;
    const std::vector<Cube>& cubes() const;

private:
    struct IndexNode {
        int variable = -1;
        int zero = -1;
        int one = -1;
        int wildcard = -1;
        std::vector<std::size_t> cube_indices;
    };

    std::vector<EndgameEntry> baseline_;
    std::vector<Cube> cubes_;
    std::vector<IndexNode> index_nodes_;
    std::size_t bit_count_ = 0;
    int root_ = -1;

    void distance_merge(std::size_t merge_distance);
    bool conflicts_with_baseline(const Cube& cube) const;
    int build_index(const std::vector<std::size_t>& cube_indices, const std::vector<int>& variables);
    int query_index(int node_index, const std::vector<int>& bits) const;
};

} // namespace endgame
} // namespace ml

#endif // ENDGAME_TABLE_COMPRESSION_H