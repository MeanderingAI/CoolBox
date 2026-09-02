#include "tyst_framework.hpp"

#include "boost_tree.h"

#include <vector>

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
