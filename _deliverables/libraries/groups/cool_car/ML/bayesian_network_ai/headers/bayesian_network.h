/**
 * @file bayesian_network.h
 * @brief Discrete Bayesian Network with exact inference (variable elimination).
 *
 * Usage:
 * @code{.cpp}
 * BayesianNetwork bn;
 * int A = bn.add_node("A", {"T", "F"});
 * int B = bn.add_node("B", {"T", "F"});
 * bn.add_edge(A, B);
 * bn.set_cpt(A, {0.6, 0.4});           // P(A=T)=0.6, P(A=F)=0.4
 * bn.set_cpt(B, {0.9, 0.1, 0.2, 0.8}); // P(B|A): rows=parent configs, cols=B states
 * auto result = bn.query(B, {{A, 0}});   // P(B | A=T)
 * @endcode
 */
#ifndef BAYESIAN_NETWORK_H
#define BAYESIAN_NETWORK_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cassert>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <functional>

/**
 * @brief A factor (potential) over a set of variables.
 *
 * Stores a flat probability table indexed by variable assignments.
 * Used internally for variable elimination.
 */
struct Factor {
    std::vector<int> variables;  ///< Variable IDs in this factor
    std::vector<int> card;       ///< Cardinality of each variable
    std::vector<double> table;   ///< Flat probability table (row-major)

    Factor() = default;

    Factor(const std::vector<int>& vars,
           const std::vector<int>& cardinalities,
           const std::vector<double>& values)
        : variables(vars), card(cardinalities), table(values) {}

    /** @brief Total number of entries in the table. */
    int size() const {
        if (card.empty()) return 1;
        int s = 1;
        for (int c : card) s *= c;
        return s;
    }

    /** @brief Convert a flat index to per-variable assignments. */
    std::vector<int> index_to_assignment(int idx) const {
        std::vector<int> assignment(variables.size());
        for (int i = static_cast<int>(variables.size()) - 1; i >= 0; --i) {
            assignment[i] = idx % card[i];
            idx /= card[i];
        }
        return assignment;
    }

    /** @brief Convert per-variable assignments to flat index. */
    int assignment_to_index(const std::vector<int>& assignment) const {
        int idx = 0;
        int stride = 1;
        for (int i = static_cast<int>(variables.size()) - 1; i >= 0; --i) {
            idx += assignment[i] * stride;
            stride *= card[i];
        }
        return idx;
    }

    /** @brief Multiply two factors (factor product). */
    static Factor multiply(const Factor& f1, const Factor& f2,
                           const std::map<int, int>& all_cards);

    /** @brief Sum out (marginalise) a variable from this factor. */
    Factor marginalise(int var) const;

    /** @brief Reduce factor by fixing a variable to a value (evidence). */
    Factor reduce(int var, int value) const;

    /** @brief Normalise table to sum to 1. */
    void normalise();
};

/**
 * @brief Discrete Bayesian Network.
 *
 * Supports:
 *   - Adding nodes (discrete random variables with named states)
 *   - Adding directed edges (parent → child)
 *   - Setting Conditional Probability Tables (CPTs)
 *   - Exact inference via variable elimination
 *   - Prior and posterior marginals
 *   - Topological ordering
 */
class BayesianNetwork {
public:
    /**
     * @brief Describes a node in the network.
     */
    struct Node {
        int id;
        std::string name;
        std::vector<std::string> states;   ///< e.g. {"T", "F"}
        std::vector<int> parents;
        std::vector<int> children;
        std::vector<double> cpt;           ///< Flat CPT table
    };

private:
    std::vector<Node> nodes_;
    std::map<std::string, int> name_to_id_;

public:
    BayesianNetwork() = default;

    // ---------------------------------------------------------------
    // Structure
    // ---------------------------------------------------------------

    /**
     * @brief Add a discrete node.
     * @param name   Unique node name.
     * @param states State labels (e.g. {"True", "False"}).
     * @return Node ID.
     */
    int add_node(const std::string& name,
                 const std::vector<std::string>& states) {
        if (states.empty())
            throw std::invalid_argument("Node must have at least one state");
        if (name_to_id_.count(name))
            throw std::invalid_argument("Duplicate node name: " + name);

        int id = static_cast<int>(nodes_.size());
        Node node;
        node.id = id;
        node.name = name;
        node.states = states;
        nodes_.push_back(node);
        name_to_id_[name] = id;
        return id;
    }

    /**
     * @brief Add a directed edge parent → child.
     * @throws std::invalid_argument if edge creates a cycle.
     */
    void add_edge(int parent, int child) {
        validate_id(parent);
        validate_id(child);
        if (parent == child)
            throw std::invalid_argument("Self-loop not allowed");

        // Check for cycle: would adding parent→child create a path child→...→parent?
        if (has_path(child, parent))
            throw std::invalid_argument("Edge would create a cycle");

        nodes_[parent].children.push_back(child);
        nodes_[child].parents.push_back(parent);
    }

    /**
     * @brief Add edge by node names.
     */
    void add_edge(const std::string& parent_name,
                  const std::string& child_name) {
        add_edge(get_node_id(parent_name), get_node_id(child_name));
    }

    // ---------------------------------------------------------------
    // CPT (Conditional Probability Table)
    // ---------------------------------------------------------------

    /**
     * @brief Set the CPT for a node.
     *
     * The table is a flat vector in row-major order where:
     *   - Rows correspond to parent configurations
     *     (ordered by parent index, fastest-changing last parent)
     *   - Columns correspond to the node's own states
     *
     * For a root node (no parents), the table is just the prior:
     *   e.g. {0.6, 0.4} for P(A=T)=0.6, P(A=F)=0.4
     *
     * For a node with parents, the table has
     *   (product of parent cardinalities) rows × (node cardinality) cols.
     *
     * @param node_id  Target node.
     * @param cpt      Flat CPT values.
     */
    void set_cpt(int node_id, const std::vector<double>& cpt) {
        validate_id(node_id);
        int expected = expected_cpt_size(node_id);
        if (static_cast<int>(cpt.size()) != expected)
            throw std::invalid_argument(
                "CPT size mismatch for node '" + nodes_[node_id].name +
                "': expected " + std::to_string(expected) +
                ", got " + std::to_string(cpt.size()));
        nodes_[node_id].cpt = cpt;
    }

    /**
     * @brief Set CPT by node name.
     */
    void set_cpt(const std::string& name, const std::vector<double>& cpt) {
        set_cpt(get_node_id(name), cpt);
    }

    // ---------------------------------------------------------------
    // Inference
    // ---------------------------------------------------------------

    /**
     * @brief Query P(query_node | evidence) via variable elimination.
     * @param query_node  The node to query.
     * @param evidence    Map of node_id → observed state index.
     * @return Probability distribution over query node's states.
     */
    std::vector<double> query(int query_node,
                              const std::map<int, int>& evidence = {}) const {
        validate_id(query_node);

        // Build cardinality map
        std::map<int, int> all_cards;
        for (const auto& n : nodes_)
            all_cards[n.id] = static_cast<int>(n.states.size());

        // Create initial factors from CPTs
        std::vector<Factor> factors;
        for (const auto& node : nodes_) {
            if (node.cpt.empty())
                throw std::runtime_error("CPT not set for node '" + node.name + "'");
            factors.push_back(make_factor(node));
        }

        // Apply evidence: reduce factors
        for (const auto& [var, val] : evidence) {
            for (auto& f : factors) {
                if (std::find(f.variables.begin(), f.variables.end(), var)
                    != f.variables.end()) {
                    f = f.reduce(var, val);
                }
            }
        }

        // Variable elimination order: eliminate all non-query, non-evidence vars
        std::vector<int> elim_order;
        for (const auto& n : nodes_) {
            if (n.id != query_node && evidence.find(n.id) == evidence.end())
                elim_order.push_back(n.id);
        }

        // Eliminate variables one by one
        for (int var : elim_order) {
            // Collect factors that mention this variable
            std::vector<Factor> relevant, remaining;
            for (auto& f : factors) {
                if (std::find(f.variables.begin(), f.variables.end(), var)
                    != f.variables.end()) {
                    relevant.push_back(f);
                } else {
                    remaining.push_back(f);
                }
            }

            if (relevant.empty()) {
                factors = remaining;
                continue;
            }

            // Multiply all relevant factors
            Factor product = relevant[0];
            for (size_t i = 1; i < relevant.size(); ++i) {
                product = Factor::multiply(product, relevant[i], all_cards);
            }

            // Marginalise out the variable
            Factor marginalised = product.marginalise(var);
            remaining.push_back(marginalised);
            factors = remaining;
        }

        // Multiply remaining factors
        Factor result = factors[0];
        for (size_t i = 1; i < factors.size(); ++i) {
            result = Factor::multiply(result, factors[i], all_cards);
        }

        // Normalise
        result.normalise();
        return result.table;
    }

    /**
     * @brief Query by node name.
     */
    std::vector<double> query(const std::string& name,
                              const std::map<std::string, std::string>& evidence = {}) const {
        int qid = get_node_id(name);
        std::map<int, int> ev;
        for (const auto& [n, s] : evidence) {
            int nid = get_node_id(n);
            int sid = get_state_index(nid, s);
            ev[nid] = sid;
        }
        return query(qid, ev);
    }

    /**
     * @brief Get the prior marginal of a node (no evidence).
     */
    std::vector<double> prior(int node_id) const {
        return query(node_id);
    }

    // ---------------------------------------------------------------
    // Topological sort
    // ---------------------------------------------------------------

    /**
     * @brief Return nodes in topological order.
     */
    std::vector<int> topological_order() const {
        std::vector<int> order;
        std::set<int> visited;
        std::set<int> in_stack;

        std::function<void(int)> dfs = [&](int id) {
            if (in_stack.count(id))
                throw std::runtime_error("Cycle detected in network");
            if (visited.count(id)) return;
            in_stack.insert(id);
            for (int child : nodes_[id].children)
                dfs(child);
            in_stack.erase(id);
            visited.insert(id);
            order.push_back(id);
        };

        for (const auto& n : nodes_)
            if (!visited.count(n.id))
                dfs(n.id);

        std::reverse(order.begin(), order.end());
        return order;
    }

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    int num_nodes() const { return static_cast<int>(nodes_.size()); }

    const Node& node(int id) const {
        validate_id(id);
        return nodes_[id];
    }

    int get_node_id(const std::string& name) const {
        auto it = name_to_id_.find(name);
        if (it == name_to_id_.end())
            throw std::invalid_argument("Unknown node: " + name);
        return it->second;
    }

    int get_state_index(int node_id, const std::string& state) const {
        validate_id(node_id);
        const auto& states = nodes_[node_id].states;
        auto it = std::find(states.begin(), states.end(), state);
        if (it == states.end())
            throw std::invalid_argument(
                "Unknown state '" + state + "' for node '" +
                nodes_[node_id].name + "'");
        return static_cast<int>(std::distance(states.begin(), it));
    }

    const std::vector<Node>& nodes() const { return nodes_; }

    /**
     * @brief Pretty-print the network structure.
     */
    void print() const {
        std::cout << "BayesianNetwork (" << nodes_.size() << " nodes)\n";
        for (const auto& n : nodes_) {
            std::cout << "  [" << n.id << "] " << n.name << " {";
            for (size_t i = 0; i < n.states.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << n.states[i];
            }
            std::cout << "}";
            if (!n.parents.empty()) {
                std::cout << " | parents: ";
                for (size_t i = 0; i < n.parents.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << nodes_[n.parents[i]].name;
                }
            }
            std::cout << "\n";
        }
    }

private:
    void validate_id(int id) const {
        if (id < 0 || id >= static_cast<int>(nodes_.size()))
            throw std::out_of_range("Invalid node ID: " + std::to_string(id));
    }

    int expected_cpt_size(int node_id) const {
        int node_card = static_cast<int>(nodes_[node_id].states.size());
        int parent_configs = 1;
        for (int pid : nodes_[node_id].parents)
            parent_configs *= static_cast<int>(nodes_[pid].states.size());
        return parent_configs * node_card;
    }

    bool has_path(int from, int to) const {
        if (from == to) return true;
        std::set<int> visited;
        std::vector<int> stack = {from};
        while (!stack.empty()) {
            int curr = stack.back();
            stack.pop_back();
            if (curr == to) return true;
            if (visited.count(curr)) continue;
            visited.insert(curr);
            for (int child : nodes_[curr].children)
                stack.push_back(child);
        }
        return false;
    }

    /**
     * @brief Build a Factor from a node's CPT.
     *
     * Factor variables: [parent0, parent1, ..., node_id]
     * Table layout matches the CPT layout.
     */
    Factor make_factor(const Node& node) const {
        std::vector<int> vars = node.parents;
        vars.push_back(node.id);

        std::vector<int> card;
        for (int v : vars)
            card.push_back(static_cast<int>(nodes_[v].states.size()));

        return Factor(vars, card, node.cpt);
    }
};

// ===================================================================
// Factor implementation
// ===================================================================

inline Factor Factor::multiply(const Factor& f1, const Factor& f2,
                               const std::map<int, int>& all_cards) {
    // Union of variables
    std::vector<int> new_vars = f1.variables;
    for (int v : f2.variables) {
        if (std::find(new_vars.begin(), new_vars.end(), v) == new_vars.end())
            new_vars.push_back(v);
    }

    std::vector<int> new_card;
    for (int v : new_vars)
        new_card.push_back(all_cards.at(v));

    Factor result(new_vars, new_card, {});
    result.table.resize(result.size(), 0.0);

    // Build index maps: for each var in new_vars, find its position in f1 and f2
    auto pos_in = [](const std::vector<int>& vars, int v) -> int {
        auto it = std::find(vars.begin(), vars.end(), v);
        return (it != vars.end()) ? static_cast<int>(std::distance(vars.begin(), it)) : -1;
    };

    for (int i = 0; i < result.size(); ++i) {
        std::vector<int> assignment = result.index_to_assignment(i);

        // Map to f1 assignment
        std::vector<int> a1(f1.variables.size());
        for (size_t j = 0; j < f1.variables.size(); ++j) {
            int p = pos_in(new_vars, f1.variables[j]);
            a1[j] = assignment[p];
        }

        // Map to f2 assignment
        std::vector<int> a2(f2.variables.size());
        for (size_t j = 0; j < f2.variables.size(); ++j) {
            int p = pos_in(new_vars, f2.variables[j]);
            a2[j] = assignment[p];
        }

        result.table[i] = f1.table[f1.assignment_to_index(a1)]
                        * f2.table[f2.assignment_to_index(a2)];
    }

    return result;
}

inline Factor Factor::marginalise(int var) const {
    auto it = std::find(variables.begin(), variables.end(), var);
    if (it == variables.end()) return *this;

    int pos = static_cast<int>(std::distance(variables.begin(), it));
    int var_card = card[pos];

    std::vector<int> new_vars, new_card;
    for (size_t i = 0; i < variables.size(); ++i) {
        if (static_cast<int>(i) != pos) {
            new_vars.push_back(variables[i]);
            new_card.push_back(card[i]);
        }
    }

    Factor result(new_vars, new_card, {});
    result.table.resize(result.size(), 0.0);

    for (int i = 0; i < this->size(); ++i) {
        std::vector<int> assignment = index_to_assignment(i);

        // Build reduced assignment (skip the marginalised variable)
        std::vector<int> reduced;
        for (size_t j = 0; j < assignment.size(); ++j) {
            if (static_cast<int>(j) != pos)
                reduced.push_back(assignment[j]);
        }

        if (!reduced.empty()) {
            int new_idx = result.assignment_to_index(reduced);
            result.table[new_idx] += table[i];
        } else {
            // All variables marginalised — single scalar
            result.table[0] += table[i];
        }
    }

    return result;
}

inline Factor Factor::reduce(int var, int value) const {
    auto it = std::find(variables.begin(), variables.end(), var);
    if (it == variables.end()) return *this;

    int pos = static_cast<int>(std::distance(variables.begin(), it));

    std::vector<int> new_vars, new_card;
    for (size_t i = 0; i < variables.size(); ++i) {
        if (static_cast<int>(i) != pos) {
            new_vars.push_back(variables[i]);
            new_card.push_back(card[i]);
        }
    }

    Factor result(new_vars, new_card, {});
    result.table.resize(result.size(), 0.0);

    for (int i = 0; i < this->size(); ++i) {
        std::vector<int> assignment = index_to_assignment(i);
        if (assignment[pos] != value) continue;

        std::vector<int> reduced;
        for (size_t j = 0; j < assignment.size(); ++j) {
            if (static_cast<int>(j) != pos)
                reduced.push_back(assignment[j]);
        }

        if (!reduced.empty()) {
            result.table[result.assignment_to_index(reduced)] = table[i];
        } else {
            result.table[0] = table[i];
        }
    }

    return result;
}

inline void Factor::normalise() {
    double sum = 0;
    for (double v : table) sum += v;
    if (sum > 0) {
        for (double& v : table) v /= sum;
    }
}

#endif // BAYESIAN_NETWORK_H
