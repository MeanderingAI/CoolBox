﻿#include "tyst_framework.hpp"

#include "decision_tree.h"

#include <vector>

namespace {

std::vector<std::vector<int>> make_training_inputs() {
	return {
		{0, 0},
		{0, 1},
		{1, 0},
		{1, 1},
		{1, 2},
	};
}

std::vector<int> make_training_labels() {
	return {0, 0, 1, 1, 1};
}

} // namespace

TYST_TEST(DecisionTreeTest, CalculatesExpectedImpurityValues) {
	const std::vector<int> labels = {0, 0, 1, 1};

	TYST_EXPECT_NEAR(calculate_gini_impurity(labels), 0.5, 1e-12);
	TYST_EXPECT_NEAR(calculate_entropy(labels), 1.0, 1e-12);
}

TYST_TEST(DecisionTreeTest, PredictsSeenAndUnknownSamples) {
	DecisionTree tree(SplitCriterion::GINI);
	const auto inputs = make_training_inputs();
	const auto labels = make_training_labels();

	tree.fit(inputs, labels, 2);

	TYST_EXPECT_EQ(tree.predict({0, 3}), 0);
	TYST_EXPECT_EQ(tree.predict({1, 4}), 1);
	TYST_EXPECT_EQ(tree.predict({2, 0}), 1);
}
