#include "tyst_framework.hpp"

#include "random_forest.h"

#include <vector>
#include <limits>
#include <stdexcept>

namespace {

std::vector<std::vector<int>> make_forest_inputs() {
	std::vector<std::vector<int>> inputs;
	for (int index = 0; index < 20; ++index) {
		inputs.push_back({0, index % 3});
		inputs.push_back({1, index % 3});
	}
	return inputs;
}

TYST_TEST(RandomForestRegressorTest, LearnsNumericTargetsAndIsReproducible) {
	const std::vector<std::vector<double>> X = {{0}, {0.1}, {0.2}, {1}, {1.1}, {1.2}};
	const std::vector<double> y = {-2.5, -2.5, -2.5, 4.5, 4.5, 4.5};
	RandomForestRegressor model(50, 2, 123);
	RandomForestRegressor repeated(50, 2, 123);
	model.fit(X, y);
	repeated.fit(X, y);
	TYST_EXPECT_NEAR(model.predict({0.05}), -2.5, 0.6);
	TYST_EXPECT_NEAR(model.predict({1.05}), 4.5, 0.6);
	TYST_EXPECT_DOUBLE_EQ(model.predict({0.5}), repeated.predict({0.5}));
	const double before = model.predict({0.5});
	model.fit(X, y);
	TYST_EXPECT_DOUBLE_EQ(model.predict({0.5}), before);
}

TYST_TEST(RandomForestRegressorTest, AveragesLeavesAndHandlesConstantData) {
	RandomForestRegressor model(200, 0, 7);
	model.fit({{0}, {1}, {2}, {3}}, {1, 3, 5, 7});
	TYST_EXPECT_NEAR(model.predict({0}), 4.0, 0.4);
	TYST_EXPECT_DOUBLE_EQ(model.predict({0}), model.predict({100}));
	model.fit({{1, 2}}, {-4.5});
	TYST_EXPECT_DOUBLE_EQ(model.predict({1, 2}), -4.5);
}

TYST_TEST(RandomForestRegressorTest, ValidatesDataAndPreservesModel) {
	TYST_EXPECT_THROW(RandomForestRegressor(0, 2), std::invalid_argument);
	TYST_EXPECT_THROW(RandomForestRegressor(2, -1), std::invalid_argument);
	RandomForestRegressor model(5, 2);
	TYST_EXPECT_THROW(model.predict({0}), std::logic_error);
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{}}, {1}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}, {1, 2}}, {1, 2}), std::invalid_argument);
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();
	TYST_EXPECT_THROW(model.fit({{nan}}, {1}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}}, {inf}), std::invalid_argument);
	model.fit({{0}}, {3.5});
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_DOUBLE_EQ(model.predict({0}), 3.5);
	TYST_EXPECT_THROW(model.predict({}), std::invalid_argument);
	TYST_EXPECT_THROW(model.predict({nan}), std::invalid_argument);
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
