﻿#include "tyst_framework.hpp"

#include "random_forest.h"

#include <vector>

namespace {

std::vector<std::vector<int>> make_forest_inputs() {
	std::vector<std::vector<int>> inputs;
	for (int index = 0; index < 20; ++index) {
		inputs.push_back({0, index % 3});
		inputs.push_back({1, index % 3});
	}
	return inputs;
}

std::vector<int> make_forest_labels() {
	std::vector<int> labels;
	for (int index = 0; index < 20; ++index) {
		labels.push_back(0);
		labels.push_back(1);
	}
	return labels;
}

} // namespace

TYST_TEST(RandomForestTest, ClassifiesSimpleSeparableSamples) {
	RandomForest forest(25, 3);
	const auto inputs = make_forest_inputs();
	const auto labels = make_forest_labels();

	forest.fit(inputs, labels);

	TYST_EXPECT_EQ(forest.predict({0, 1}), 0);
	TYST_EXPECT_EQ(forest.predict({1, 2}), 1);
}
