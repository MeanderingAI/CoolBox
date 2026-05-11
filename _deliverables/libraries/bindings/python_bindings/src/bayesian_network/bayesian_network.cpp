
#include <iostream>
#include <numeric>
#include <algorithm>
#include <bayesian_network.h>

// Constructor
BayesianNetwork::BayesianNetwork() {}

/**
 * @brief Adds a new node (random variable) to the network.
 * @param node_name The name of the new node.
 * @param states The possible discrete states for this node.
 * @return The index of the newly added node.
 */
int BayesianNetwork::add_node(const std::string& node_name, const std::vector<std::string>& states) {
    int new_index = nodes_.size();
    nodes_.push_back({node_name, states, {}, new_index});
    return new_index;
}

/**
 * @brief Adds a directed edge from a parent node to a child node.
 * @param parent_index The index of the parent node.
 * @param child_index The index of the child node.
 */
void BayesianNetwork::add_edge(int parent_index, int child_index) {
    if (child_index >= 0 && child_index < nodes_.size()) {
        nodes_[child_index].parents.insert(parent_index);
    }
}

/**
 * @brief Sets the conditional probability table (CPT) for a given node.
 * @param node_index The index of the node for which to set the CPT.
 * @param cpt The conditional probability table matrix.
 */
void BayesianNetwork::set_cpt(int node_index, const Eigen::MatrixXd& cpt) {
    cpts_by_node_index_[node_index] = cpt;
}

// Private helper for topological sorting
void dfs_topological_sort(int node_index, const std::vector<BayesianNetwork::Node>& nodes, std::set<int>& visited, std::vector<int>& sorted_list) {
    visited.insert(node_index);
    
    // Find children of the current node
    for (const auto& child_node : nodes) {
        if (child_node.parents.count(node_index)) {
            if (visited.find(child_node.index) == visited.end()) {
                dfs_topological_sort(child_node.index, nodes, visited, sorted_list);
            }
        }
    }
    sorted_list.push_back(node_index);
}

// Helper to get the index into a CPT based on parent assignments
int get_cpt_row_index(int node_index, const std::map<int, int>& assignment, const std::vector<BayesianNetwork::Node>& nodes) {
    int row_index = 0;
    int multiplier = 1;
    
    for (int parent_index : nodes[node_index].parents) {
        row_index += assignment.at(parent_index) * multiplier;
        multiplier *= nodes[parent_index].states.size();
    }
    return row_index;
}

/**
 * @brief Calculates the joint probability of a complete assignment of states.
 * @param assignment A map from node index to the state index for that node.
 * @return The joint probability of the given assignment.
 */
double BayesianNetwork::calculate_joint_probability(const std::map<int, int>& assignment) const {
    if (assignment.size() != nodes_.size()) {
        std::cerr << "Error: Assignment is not complete." << std::endl;
        return 0.0;
    }
    
    // Perform a topological sort to ensure correct calculation order
    std::set<int> visited;
    std::vector<int> sorted_nodes;
    for (int i = 0; i < nodes_.size(); ++i) {
        if (visited.find(i) == visited.end()) {
            dfs_topological_sort(i, nodes_, visited, sorted_nodes);
        }
    }
    std::reverse(sorted_nodes.begin(), sorted_nodes.end());

    double total_probability = 1.0;
    
    // Iterate through nodes in topological order
    for (int node_index : sorted_nodes) {
        const auto& node = nodes_[node_index];
        const auto& cpt = cpts_by_node_index_.at(node_index);
        
        // Find the correct probability from the CPT
        int row_index = get_cpt_row_index(node_index, assignment, nodes_);
        int col_index = assignment.at(node_index);
        
        total_probability *= cpt(row_index, col_index);
    }
    
    return total_probability;
}

// Private helper to recursively generate and process assignments for hidden variables
void infer_recursive(
    int hidden_index,
    const std::vector<int>& hidden_nodes,
    const BayesianNetwork& network,
    std::map<int, int> current_assignment,
    double& numerator_prob,
    double& denominator_prob,
    int query_node_index,
    int query_state_index
) {
    if (hidden_index == hidden_nodes.size()) {
        // Base case: a full assignment for all hidden nodes has been created.
        // Now, we must iterate through all possible query states to find the
        // total probabilities for the numerator and denominator.
        
        // Calculate the denominator by summing the probabilities for all possible query states
        double denominator_sum = 0.0;
        int num_query_states = network.nodes_[query_node_index].states.size();
        for (int state_idx = 0; state_idx < num_query_states; ++state_idx) {
            std::map<int, int> temp_assignment = current_assignment;
            temp_assignment[query_node_index] = state_idx;
            denominator_sum += network.calculate_joint_probability(temp_assignment);
        }

        // Calculate the numerator by getting the probability for the specific query state
        std::map<int, int> numerator_assignment = current_assignment;
        numerator_assignment[query_node_index] = query_state_index;
        double numerator_sum = network.calculate_joint_probability(numerator_assignment);

        // Add to the total probabilities
        numerator_prob += numerator_sum;
        denominator_prob += denominator_sum;

        return;
    }

    // Recursive step: iterate through states of the current hidden node
    int current_hidden_node_index = hidden_nodes[hidden_index];
    int num_states = network.nodes_[current_hidden_node_index].states.size();
    
    for (int state_index = 0; state_index < num_states; ++state_index) {
        current_assignment[current_hidden_node_index] = state_index;
        infer_recursive(
            hidden_index + 1,
            hidden_nodes,
            network,
            current_assignment,
            numerator_prob,
            denominator_prob,
            query_node_index,
            query_state_index
        );
    }
}

// Private helper to recursively generate and process assignments for hidden variables
double sum_over_hidden_recursive(
    int hidden_index,
    const std::vector<int>& hidden_nodes,
    const BayesianNetwork& network,
    std::map<int, int> current_assignment
) {
    if (hidden_index == hidden_nodes.size()) {
        // Base case: all hidden nodes have been assigned states.
        // Calculate and return the joint probability of this full assignment.
        return network.calculate_joint_probability(current_assignment);
    }

    // Recursive step: iterate through states of the current hidden node
    double total_prob = 0.0;
    int current_hidden_node_index = hidden_nodes[hidden_index];
    int num_states = network.nodes_[current_hidden_node_index].states.size();
    
    for (int state_index = 0; state_index < num_states; ++state_index) {
        current_assignment[current_hidden_node_index] = state_index;
        total_prob += sum_over_hidden_recursive(
            hidden_index + 1,
            hidden_nodes,
            network,
            current_assignment
        );
    }
    return total_prob;
}


/**
 * @brief Performs inference to find the probability of a query given evidence.
 * @param query_node_index The index of the node whose probability is to be calculated.
 * @param query_state_index The index of the state for the query node.
 * @param evidence A map from node index to the state index of the observed evidence.
 * @return The conditional probability P(query | evidence).
 */
double BayesianNetwork::infer(int query_node_index, int query_state_index, const std::map<int, int>& evidence) const {
    // Identify hidden nodes (all nodes not in the evidence)
    std::vector<int> hidden_nodes_with_query;
    for (int i = 0; i < nodes_.size(); ++i) {
        if (evidence.find(i) == evidence.end()) {
            hidden_nodes_with_query.push_back(i);
        }
    }

    // Calculate the denominator: P(evidence) = sum_{all_nodes_not_in_evidence} P(all_nodes_not_in_evidence, evidence)
    double denominator = sum_over_hidden_recursive(
        0, 
        hidden_nodes_with_query,
        *this,
        evidence
    );

    // Calculate the numerator: P(query, evidence) = sum_{all_nodes_not_in_evidence_or_query} P(query, evidence, all_nodes_not_in_evidence_or_query)
    std::vector<int> hidden_nodes_without_query;
    for (int i : hidden_nodes_with_query) {
        if (i != query_node_index) {
            hidden_nodes_without_query.push_back(i);
        }
    }
    
    std::map<int, int> numerator_assignment = evidence;
    numerator_assignment[query_node_index] = query_state_index;

    double numerator = sum_over_hidden_recursive(
        0,
        hidden_nodes_without_query,
        *this,
        numerator_assignment
    );
    
    // The conditional probability is P(query, evidence) / P(evidence)
    if (denominator == 0.0) {
        return 0.0;
    }
    return numerator / denominator;
}