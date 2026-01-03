#ifndef RULE_SET_H
#define RULE_SET_H

#include <string>
#include <vector>
#include <map>
#include <decision_tree.h>

class RuleSet {
public:
    /**
     * @brief Constructor that generates a rule set from a trained decision tree.
     * @param tree The decision tree instance to convert to rules.
     */
    RuleSet(const DecisionTree& tree);

    /**
     * @brief Retrieves the vector of rules.
     * @return A vector of strings, where each string is a single decision rule.
     */
    const std::vector<std::string>& get_rules() const;

    /**
     * @brief Predicts the class label for a single sample using the rule set.
     * @param sample The feature vector for the sample to classify.
     * @return The predicted class label. Returns -1 if no rule is matched.
     */
    int predict(const std::vector<int>& sample) const;

private:
    // Represents a single, parsed decision rule.
    struct Rule {
        // Map of feature index to required value.
        std::map<int, int> conditions;
        // The predicted class label for this rule.
        int class_label;
    };
    
    std::vector<std::string> rules_as_strings;
    std::vector<Rule> parsed_rules_;
    
    // Helper function to recursively convert the tree to a rule set.
    void to_rules_recursive(const Node* node, std::map<int, int> current_conditions);

    // Helper to check if a sample satisfies a given rule.
    bool match_rule(const std::vector<int>& sample, const Rule& rule) const;
};

#endif // RULE_SET_H