#include "tyst_framework.hpp"

#include "decision_tree.h"

#include <vector>
#include <limits>
#include <stdexcept>

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

TYST_TEST(DecisionTreeTest, PreservesEntropyAndMulticlassClassification) {
	DecisionTree tree(SplitCriterion::ENTROPY);
	tree.fit({{0}, {1}, {2}, {2}}, {-3, 4, 9, 9}, 1);
	TYST_EXPECT_EQ(tree.predict({0}), -3);
	TYST_EXPECT_EQ(tree.predict({1}), 4);
	TYST_EXPECT_EQ(tree.predict({2}), 9);
	TYST_EXPECT_EQ(tree.predict({3}), 9);
}

TYST_TEST(DecisionTreeRegressorTest, SplitsNumericFeaturesAndPredictsUnseenSamples) {
	DecisionTreeRegressor tree;
	tree.fit({{0.1}, {0.3}, {0.7}, {0.9}}, {-2.5, -2.5, 4.5, 4.5}, 1);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.2}), -2.5);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.5}), -2.5);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.8}), 4.5);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({-100.0}), -2.5);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({100.0}), 4.5);
}

TYST_TEST(DecisionTreeRegressorTest, ChoosesLowestSquaredErrorAcrossFeatures) {
	DecisionTreeRegressor tree;
	tree.fit({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
	         {0.0, 2.0, 10.0, 12.0}, 1);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({1.0, 0.0}), 1.0);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0, 1.0}), 11.0);
	tree.fit({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
	         {0.0, 2.0, 10.0, 12.0}, 2);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({1.0, 0.0}), 2.0);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0, 1.0}), 10.0);
}

TYST_TEST(DecisionTreeRegressorTest, HandlesZeroDepthConstantFeaturesAndRepeatedValues) {
	DecisionTreeRegressor tree;
	tree.fit({{0.0}, {1.0}, {2.0}}, {1.0, 2.0, 6.0}, 0);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({100.0}), 3.0);
	tree.fit({{1.0}, {1.0}, {1.0}}, {1.0, 2.0, 6.0}, 100);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({1.0}), 3.0);
	tree.fit({{0.0}, {0.0}, {1.0}, {1.0}}, {1.0, 3.0, 8.0, 10.0}, 3);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0}), 2.0);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({1.0}), 9.0);
	tree.fit({{0.0}, {1.0}}, {2.5, 2.5}, 10);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.5}), 2.5);
	tree.fit({{0.0}}, {-3.5}, 10);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({1.0}), -3.5);
}

TYST_TEST(DecisionTreeRegressorTest, HandlesInteractionsWithZeroInitialGain) {
	DecisionTreeRegressor tree;
	const std::vector<std::vector<double>> X = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
	const std::vector<double> y = {0.5, 2.5, 2.5, 0.5};
	tree.fit(X, y, 2);
	for (std::size_t i = 0; i < X.size(); ++i) {
		TYST_EXPECT_DOUBLE_EQ(tree.predict(X[i]), y[i]);
	}
}

TYST_TEST(DecisionTreeRegressorTest, HandlesLargeTargetsAndAdjacentFeatureValues) {
	DecisionTreeRegressor tree;
	const double lower = 1e308;
	const double upper = std::nextafter(lower, std::numeric_limits<double>::infinity());
	tree.fit({{lower}, {upper}}, {1e12 + 1.0, 1e12 + 3.0}, 1);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({lower}), 1e12 + 1.0);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({upper}), 1e12 + 3.0);
	tree.fit({{-1e308}, {1e308}}, {-2.0, 4.0}, 1);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0}), -2.0);
}

TYST_TEST(DecisionTreeRegressorTest, RejectsInvalidDataAndPreservesModelOnFailedRefit) {
	DecisionTreeRegressor tree;
	TYST_EXPECT_THROW(tree.predict({0.0}), std::logic_error);
	TYST_EXPECT_THROW(tree.fit({}, {}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{0.0}}, {}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{}}, {1.0}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{0.0}, {1.0, 2.0}}, {1.0, 2.0}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{0.0}}, {1.0}, -1), std::invalid_argument);
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();
	TYST_EXPECT_THROW(tree.fit({{nan}}, {1.0}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{inf}}, {1.0}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{0.0}}, {nan}, 1), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({{0.0}}, {inf}, 1), std::invalid_argument);
	tree.fit({{0.0}}, {7.5}, 1);
	TYST_EXPECT_THROW(tree.predict({}), std::invalid_argument);
	TYST_EXPECT_THROW(tree.predict({0.0, 1.0}), std::invalid_argument);
	TYST_EXPECT_THROW(tree.predict({nan}), std::invalid_argument);
	TYST_EXPECT_THROW(tree.predict({inf}), std::invalid_argument);
	TYST_EXPECT_THROW(tree.fit({}, {}, 1), std::invalid_argument);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0}), 7.5);
	tree.fit({{0.0, 1.0}}, {-4.5}, 1);
	TYST_EXPECT_DOUBLE_EQ(tree.predict({0.0, 1.0}), -4.5);
}
