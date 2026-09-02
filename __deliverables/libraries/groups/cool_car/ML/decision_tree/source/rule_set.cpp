#include "rule_set.h"
#include <sstream>

RuleSet::RuleSet(const DecisionTree& tree) {
    if (tree.root) {
        std::map<int, int> empty_conditions;
        to_rules_recursive(tree.root, empty_conditions);
    }
}

const std::vector<std::string>& RuleSet::get_rules() const {
    return rules_as_strings;
}

int RuleSet::predict(const std::vector<int>& sample) const {
    for (const auto& rule : parsed_rules_) {
        if (match_rule(sample, rule))
            return rule.class_label;
    }
    return -1; // No rule matched
}

void RuleSet::to_rules_recursive(const Node* node, std::map<int, int> current_conditions) {
    if (!node) return;

    if (node->is_leaf) {
        // Build rule string
        std::ostringstream oss;
        oss << "IF ";
        bool first = true;
        for (const auto& [feature, value] : current_conditions) {
            if (!first) oss << " AND ";
            oss << "feature[" << feature << "] == " << value;
            first = false;
        }
        if (current_conditions.empty()) {
            oss << "TRUE";
        }
        oss << " THEN class = " << node->class_label;
        rules_as_strings.push_back(oss.str());

        // Store parsed rule
        Rule rule;
        rule.conditions = current_conditions;
        rule.class_label = node->class_label;
        parsed_rules_.push_back(rule);
        return;
    }

    for (const auto& [value, child] : node->children) {
        auto new_conditions = current_conditions;
        new_conditions[node->feature_index] = value;
        to_rules_recursive(child, new_conditions);
    }
}

bool RuleSet::match_rule(const std::vector<int>& sample, const Rule& rule) const {
    for (const auto& [feature, value] : rule.conditions) {
        if (feature < 0 || feature >= static_cast<int>(sample.size()))
            return false;
        if (sample[feature] != value)
            return false;
    }
    return true;
}
