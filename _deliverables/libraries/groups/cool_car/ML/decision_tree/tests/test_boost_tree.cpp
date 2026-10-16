#include "tyst_framework.hpp"

#include "boost_tree.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

TYST_TEST(BoostTreeTest, LearnsHigherPredictionsForHigherFeatureValues) {
	BoostTreeParameters parameters;
	parameters.num_estimators = 25;
	parameters.learning_rate = 0.2;
	parameters.max_depth = 2;

	BoostTree model(parameters);
	const std::vector<std::vector<double>> inputs = {
		{0.0},
		{0.1},
		{0.2},
		{1.0},
		{1.1},
		{1.2},
	};
	const std::vector<double> targets = {1.0, 1.0, 1.2, 3.0, 3.1, 3.2};

	model.fit(inputs, targets);

	const double low_prediction = model.predict(std::vector<double>{0.05});
	const double high_prediction = model.predict(std::vector<double>{1.05});
	const auto batch_predictions = model.predict(
		std::vector<std::vector<double>>{{0.05}, {1.05}});

	TYST_EXPECT_EQ(batch_predictions.size(), 2u);
	TYST_EXPECT_LT(low_prediction, high_prediction);
	TYST_EXPECT_NEAR(batch_predictions[0], low_prediction, 1e-12);
	TYST_EXPECT_NEAR(batch_predictions[1], high_prediction, 1e-12);
}

TYST_TEST(BoostTreeTest, UsesConfiguredDepthForInteractions) {
	const std::vector<std::vector<double>> X = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
	const std::vector<double> y = {0, 2, 2, 0};
	BoostTreeParameters parameters;
	parameters.num_estimators = 1;
	parameters.learning_rate = 1.0;
	parameters.max_depth = 1;
	BoostTree shallow(parameters);
	shallow.fit(X, y);
	TYST_EXPECT_NEAR(shallow.predict(X[0]), 1.0, 1e-12);
	parameters.max_depth = 2;
	BoostTree deep(parameters);
	deep.fit(X, y);
	for (std::size_t i = 0; i < X.size(); ++i) {
		TYST_EXPECT_NEAR(deep.predict(X[i]), y[i], 1e-12);
	}
	parameters.max_depth = 0;
	BoostTree constant(parameters);
	constant.fit(X, y);
	TYST_EXPECT_NEAR(constant.predict(X[1]), 1.0, 1e-12);
}

TYST_TEST(BoostTreeTest, CopiedModelsRefitIndependently) {
	BoostTreeParameters parameters;
	BoostTree original(parameters);
	original.fit({{0}}, {3.5});
	BoostTree copy = original;
	original.fit({{0}}, {-4.5});
	TYST_EXPECT_DOUBLE_EQ(copy.predict(std::vector<double>{0}), 3.5);
	TYST_EXPECT_DOUBLE_EQ(original.predict(std::vector<double>{0}), -4.5);
}

TYST_TEST(BoostTreeClassifierTest, LearnsBinaryLabelsAndNormalizedProbabilities) {
	BoostTreeParameters parameters;
	parameters.num_estimators = 50;
	parameters.learning_rate = 0.3;
	BoostTreeClassifier model(parameters);
	model.fit({{-2}, {-1}, {1}, {2}}, {-7, -7, 42, 42});
	TYST_EXPECT_EQ(model.classes(), (std::vector<int>{-7, 42}));
	TYST_EXPECT_EQ(model.predict({-1.5}), -7);
	TYST_EXPECT_EQ(model.predict({1.5}), 42);
	const auto probabilities = model.predict_proba({1.5});
	TYST_EXPECT_EQ(probabilities.size(), 2u);
	TYST_EXPECT_GT(probabilities[1], 0.9);
	TYST_EXPECT_NEAR(std::accumulate(probabilities.begin(), probabilities.end(), 0.0), 1.0, 1e-12);
	for (double probability : probabilities) {
		TYST_EXPECT_GE(probability, 0.0);
		TYST_EXPECT_LE(probability, 1.0);
	}
}

TYST_TEST(BoostTreeClassifierTest, LearnsMulticlassAndRefitsWithNewClasses) {
	BoostTreeParameters parameters;
	parameters.num_estimators = 60;
	parameters.learning_rate = 0.3;
	parameters.max_depth = 2;
	BoostTreeClassifier model(parameters);
	const std::vector<std::vector<double>> X = {{-3}, {-2}, {0}, {0.1}, {2}, {3}};
	const std::vector<int> y = {10, 10, -5, -5, 99, 99};
	model.fit(X, y);
	TYST_EXPECT_EQ(model.classes(), (std::vector<int>{-5, 10, 99}));
	double log_loss = 0.0;
	for (std::size_t i = 0; i < X.size(); ++i) {
		TYST_EXPECT_EQ(model.predict(X[i]), y[i]);
		const auto probabilities = model.predict_proba(X[i]);
		const auto label = std::find(model.classes().begin(), model.classes().end(), y[i]);
		log_loss -= std::log(probabilities[label - model.classes().begin()]);
		TYST_EXPECT_NEAR(std::accumulate(probabilities.begin(), probabilities.end(), 0.0), 1.0, 1e-12);
	}
	TYST_EXPECT_LT(log_loss / X.size(), 0.15);
	model.fit({{0, 1}, {1, 0}}, {8, 8});
	TYST_EXPECT_EQ(model.classes(), (std::vector<int>{8}));
	TYST_EXPECT_EQ(model.predict({-100, 100}), 8);
	TYST_EXPECT_DOUBLE_EQ(model.predict_proba({0, 1})[0], 1.0);
}

TYST_TEST(BoostTreeClassifierTest, ZeroDepthPreservesClassPriors) {
	BoostTreeParameters parameters;
	parameters.max_depth = 0;
	BoostTreeClassifier model(parameters);
	model.fit({{0}, {1}, {2}, {3}}, {5, 5, 5, 9});
	const auto probabilities = model.predict_proba({2});
	TYST_EXPECT_NEAR(probabilities[0], 0.75, 1e-12);
	TYST_EXPECT_NEAR(probabilities[1], 0.25, 1e-12);
}

TYST_TEST(BoostTreeTest, RejectsInvalidParametersAndTrainingWithoutLosingModel) {
	BoostTreeParameters parameters;
	parameters.num_estimators = 0;
	TYST_EXPECT_THROW((void)BoostTree(parameters), std::invalid_argument);
	TYST_EXPECT_THROW((void)BoostTreeClassifier(parameters), std::invalid_argument);
	parameters.num_estimators = 2;
	parameters.learning_rate = 0.0;
	TYST_EXPECT_THROW((void)BoostTree(parameters), std::invalid_argument);
	parameters.learning_rate = std::numeric_limits<double>::infinity();
	TYST_EXPECT_THROW((void)BoostTreeClassifier(parameters), std::invalid_argument);
	parameters.learning_rate = -1.0;
	TYST_EXPECT_THROW((void)BoostTree(parameters), std::invalid_argument);
	parameters.learning_rate = 0.1;
	parameters.max_depth = std::numeric_limits<unsigned int>::max();
	TYST_EXPECT_THROW((void)BoostTreeClassifier(parameters), std::invalid_argument);
	parameters.max_depth = 2;
	BoostTree model(parameters);
	TYST_EXPECT_THROW(model.predict(std::vector<double>{0}), std::logic_error);
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{}}, {1}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}, {1, 2}}, {1, 2}), std::invalid_argument);
	const double nan = std::numeric_limits<double>::quiet_NaN();
	TYST_EXPECT_THROW(model.fit({{0}}, {nan}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{nan}}, {1}), std::invalid_argument);
	model.fit({{0}}, {4.5});
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_DOUBLE_EQ(model.predict(std::vector<double>{0}), 4.5);
	TYST_EXPECT_THROW(model.predict(std::vector<double>{}), std::invalid_argument);
	TYST_EXPECT_THROW(model.predict(std::vector<double>{nan}), std::invalid_argument);
}

TYST_TEST(BoostTreeClassifierTest, RejectsInvalidDataAndPreservesModel) {
	BoostTreeClassifier model;
	TYST_EXPECT_THROW(model.predict({0}), std::logic_error);
	TYST_EXPECT_THROW(model.predict_proba({0}), std::logic_error);
	TYST_EXPECT_THROW(model.classes(), std::logic_error);
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}}, {}), std::invalid_argument);
	TYST_EXPECT_THROW(model.fit({{0}, {1, 2}}, {1, 2}), std::invalid_argument);
	const double nan = std::numeric_limits<double>::quiet_NaN();
	TYST_EXPECT_THROW(model.fit({{nan}}, {1}), std::invalid_argument);
	model.fit({{0}}, {-8});
	TYST_EXPECT_THROW(model.fit({}, {}), std::invalid_argument);
	TYST_EXPECT_EQ(model.predict({0}), -8);
	TYST_EXPECT_THROW(model.predict({}), std::invalid_argument);
	TYST_EXPECT_THROW(model.predict_proba({nan}), std::invalid_argument);
}
