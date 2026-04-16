﻿#include "tyst_framework.hpp"

#include "decision_tree.h"
#include "rule_set.h"

#include <algorithm>
#include <string>

TYST_TEST(RuleSetTest, ConvertsTreeIntoUsableRules) {
	DecisionTree tree(SplitCriterion::GINI);
	tree.fit({{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {0, 0, 1, 1}, 2);
	RuleSet rules(tree);

	const auto& rendered_rules = rules.get_rules();
	const bool has_class_assignment = std::any_of(
		rendered_rules.begin(), rendered_rules.end(),
		[](const std::string& rule) { return rule.find("THEN class =") != std::string::npos; });

	TYST_EXPECT_EQ(rendered_rules.size(), 2u);
	TYST_EXPECT_TRUE(has_class_assignment);
	TYST_EXPECT_EQ(rules.predict({0, 9}), 0);
	TYST_EXPECT_EQ(rules.predict({1, 9}), 1);
	TYST_EXPECT_EQ(rules.predict({2, 9}), -1);
}
