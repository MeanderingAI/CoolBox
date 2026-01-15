#include <rule_set.h>
#include <sstream>

// Helper function to recursively convert the tree to a rule set.
void RuleSet::to_rules_recursive(const Node* node, std::map<int, int> current_conditions) {
    if (!node) {
        return;
    }

    if (node->is_leaf) {
        // Base case: at a leaf node, create a rule
        Rule parsed_rule;
        parsed_rule.conditions = current_conditions;
        parsed_rule.class_label = node->class_label;
        parsed_rules_.push_back(parsed_rule);

        // Convert the rule to a human-readable string
        std::stringstream ss;
        ss << "IF ";
        bool first_condition = true;
        for (const auto& pair : current_conditions) {
            if (!first_condition) {
                ss << " AND ";
            }
            ss << "feature[" << pair.first << "] == " << pair.second;
            first_condition = false;
        }
        ss << " THEN class is " << node->class_label;
        rules_as_strings.push_back(ss.str());

        return;
    }

    // Recursive step: for an internal node, traverse all children
    for (const auto& child_pair : node->children) {
        std::map<int, int> next_conditions = current_conditions;
        next_conditions[node->feature_index] = child_pair.first;
        to_rules_recursive(child_pair.second, next_conditions);
    }
}

/**
 * @brief Constructor that generates a rule set from a trained decision tree.
 * @param tree The decision tree instance to convert to rules.
 */
RuleSet::RuleSet(const DecisionTree& tree) {
    // Check if the tree is valid before trying to convert it
    if (tree.root) {
        to_rules_recursive(tree.root, {});
    }
}

/**
 * @brief Retrieves the vector of rules.
 * @return A vector of strings, where each string is a single decision rule.
 */
const std::vector<std::string>& RuleSet::get_rules() const {
    return rules_as_strings;
}

/**
 * @brief Helper to check if a sample satisfies a given rule.
 */
bool RuleSet::match_rule(const std::vector<int>& sample, const Rule& rule) const {
    for (const auto& condition : rule.conditions) {
        // Ensure the sample has the feature and it matches the condition
        if (condition.first >= sample.size() || sample[condition.first] != condition.second) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Predicts the class label for a single sample using the rule set.
 * @param sample The feature vector for the sample to classify.
 * @return The predicted class label. Returns -1 if no rule is matched.
 */
int RuleSet::predict(const std::vector<int>& sample) const {
    for (const auto& rule : parsed_rules_) {
        if (match_rule(sample, rule)) {
            return rule.class_label;
        }
    }
    return -1; // Return -1 if no rule matches the sample
}