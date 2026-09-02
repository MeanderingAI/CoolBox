#include "tyst_framework.hpp"
#include "linear_regression.h"

TEST(LinearRegression, ClosedFormPerfectFit) {
	LinearRegressionFitMethod method(0, 0.0, LinearRegressionFitMethod::CLOSED_FORM);
	LinearRegression model(method);

	// y = 2*x + 1
	std::vector<std::vector<double>> X = {{0.0}, {1.0}, {2.0}, {3.0}};
	std::vector<double> y = {1.0, 3.0, 5.0, 7.0};

	model.fit(X, y);
	auto coeffs = model.get_coefficients();
	const std::vector<double>& weights = coeffs.first;
	double bias = coeffs.second;

	ASSERT_EQ(weights.size(), 1u);
	EXPECT_NEAR(weights[0], 2.0, 1e-9);
	EXPECT_NEAR(bias, 1.0, 1e-9);

	// predictions
	EXPECT_NEAR(model.predict({4.0}), 9.0, 1e-8);
}

TEST(LinearRegression, GradientDescentApproximate) {
	LinearRegressionFitMethod method(5000, 0.01, LinearRegressionFitMethod::GRADIENT_DESCENT);
	LinearRegression model(method);

	// y = -1.5*x + 0.5
	std::vector<std::vector<double>> X;
	std::vector<double> y;
	for (int i = 0; i < 50; ++i) {
		double x = i * 0.1;
		X.push_back({x});
		y.push_back(-1.5 * x + 0.5);
	}

	model.fit(X, y);
	auto coeffs = model.get_coefficients();
	const std::vector<double>& weights = coeffs.first;
	double bias = coeffs.second;

	ASSERT_EQ(weights.size(), 1u);
	EXPECT_NEAR(weights[0], -1.5, 5e-2);
	EXPECT_NEAR(bias, 0.5, 5e-2);
}

