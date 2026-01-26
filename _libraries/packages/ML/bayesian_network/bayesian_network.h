
/**
 * @mainpage BayesianNetwork Library
 *
 * @section usage_examples Usage Examples
 *
 * @subsection cpp_example C++ Example
 * @code{.cpp}
 * #include "ml/bayesian_network/bayesian_network.h"
 * BayesianNetwork bn;
 * int nodeA = bn.add_node("A", {"T", "F"});
 * int nodeB = bn.add_node("B", {"T", "F"});
 * bn.add_edge(nodeA, nodeB);
 * // ... set CPTs, perform inference ...
 * @endcode
 *
 * @subsection python_example Python Example
 * @code{.python}
 * from ml_core.bayesian_network import BayesianNetwork
 * bn = BayesianNetwork()
 * nodeA = bn.add_node("A", ["T", "F"])
 * nodeB = bn.add_node("B", ["T", "F"])
 * bn.add_edge(nodeA, nodeB)
 * # ... set CPTs, perform inference ...
 * @endcode
 *
 * @subsection js_example JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Async usage (MODULARIZE=1, default):
 * createBayesianNetworkModule().then(Module => {
 *     const BayesianNetwork = Module.BayesianNetwork;
 *     const bn = new BayesianNetwork();
 *     const nodeA = bn.add_node("A", ["T", "F"]);
 *     const nodeB = bn.add_node("B", ["T", "F"]);
 *     bn.add_edge(nodeA, nodeB);
 *     // ... set CPTs, perform inference ...
 * });
 * @endcode
 *
 * @subsection js_example_sync JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If bayesian_network.js is loaded and exposes 'Module' globally:
 * const BayesianNetwork = Module.BayesianNetwork;
 * const bn = new BayesianNetwork();
 * const nodeA = bn.add_node("A", ["T", "F"]);
 * const nodeB = bn.add_node("B", ["T", "F"]);
 * bn.add_edge(nodeA, nodeB);
 * // ... set CPTs, perform inference ...
 * // Note: If built with MODULARIZE=1 (default), you must use createBayesianNetworkModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */
#ifndef BAYESIAN_NETWORK_H
#define BAYESIAN_NETWORK_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <Eigen/Dense>

/**
 * @class BayesianNetwork
 * @brief Represents a directed acyclic graph (DAG) where nodes are random variables
 * and edges represent conditional dependencies.
 * * This class provides the interface for defining the structure and parameters
 * of a Bayesian network and for performing probabilistic inference.
 */
class BayesianNetwork {
public:
    /**
     * @struct Node
     * @brief A structure to hold information about each node in the network.
     */
    struct Node {
        std::string name;
        std::vector<std::string> states;
        std::set<int> parents; // Indices of parent nodes
        int index;
    };

    // Constructor
    BayesianNetwork();

    /**
     * @brief Adds a new node (random variable) to the network.
     * @param node_name The name of the new node.
     * @param states The possible discrete states for this node.
     * @return The index of the newly added node.
     */
    int add_node(const std::string& node_name, const std::vector<std::string>& states);

    /**
     * @brief Adds a directed edge from a parent node to a child node.
     * @param parent_index The index of the parent node.
     * @param child_index The index of the child node.
     */
    void add_edge(int parent_index, int child_index);

    /**
     * @brief Sets the conditional probability table (CPT) for a given node.
     * * The CPT is represented by an Eigen::MatrixXd, where the rows correspond to
     * all combinations of parent states and the columns correspond to the child's states.
     * @param node_index The index of the node for which to set the CPT.
     * @param cpt The conditional probability table matrix.
     */
    void set_cpt(int node_index, const Eigen::MatrixXd& cpt);

    /**
     * @brief Calculates the joint probability of a complete assignment of states.
     * * This method requires an assignment for every node in the network.
     * @param assignment A map from node index to the state index for that node.
     * @return The joint probability of the given assignment.
     */
    double calculate_joint_probability(const std::map<int, int>& assignment) const;

    /**
     * @brief Performs inference to find the probability of a query given evidence.
     * * This method uses a simple enumeration-based approach for inference.
     * @param query_node_index The index of the node whose probability is to be calculated.
     * @param query_state_index The index of the state for the query node.
     * @param evidence A map from node index to the state index of the observed evidence.
     * @return The conditional probability P(query | evidence).
     */
    double infer(int query_node_index, int query_state_index, const std::map<int, int>& evidence) const;

private:
    std::vector<Node> nodes_;
    std::vector<std::vector<double>> cpts_; // Flattened CPTs for each node
    std::map<int, Eigen::MatrixXd> cpts_by_node_index_;

    // Declare helper functions as friends to grant them access to private members
    friend void dfs_topological_sort(int node_index, const BayesianNetwork& network, std::set<int>& visited, std::vector<int>& sorted_list);
    friend int get_cpt_row_index(int node_index, const std::map<int, int>& assignment, const BayesianNetwork& network);
    friend void infer_recursive(
        int hidden_index,
        const std::vector<int>& hidden_nodes,
        const BayesianNetwork& network,
        std::map<int, int> current_assignment,
        double& numerator_prob,
        double& denominator_prob,
        int query_node_index,
        int query_state_index
    );
    friend double sum_over_hidden_recursive(
        int hidden_index,
        const std::vector<int>& hidden_nodes,
        const BayesianNetwork& network,
        std::map<int, int> current_assignment
    );
};

#endif // BAYESIAN_NETWORK_H
