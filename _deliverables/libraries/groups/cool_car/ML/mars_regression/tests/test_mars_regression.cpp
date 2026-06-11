#include "tyst_framework.hpp"

#include "mars_regression.h"

#include <cmath>
#include <vector>

TEST(MarsRegression, FitsPiecewiseLinearSignal) {
    std::vector<std::vector<double>> X;
    std::vector<double> y;

    for (int i = 0; i <= 60; ++i) {
        const double x = -3.0 + 0.1 * static_cast<double>(i);
        const double target = 1.5 + 0.8 * x + 1.2 * std::max(0.0, x - 0.5) - 0.6 * std::max(0.0, -x - 1.0);
        X.push_back({x});
        y.push_back(target);
    }

    MarsRegressionFitMethod method(10, 3, 20, 1e-8);
    MarsRegression model(method);
    model.fit(X, y);

    EXPECT_GE(model.num_terms(), 1);

    const double pred1 = model.predict({1.5});
    const double true1 = 1.5 + 0.8 * 1.5 + 1.2 * std::max(0.0, 1.5 - 0.5) - 0.6 * std::max(0.0, -1.5 - 1.0);
    EXPECT_NEAR(pred1, true1, 0.2);

    const double pred2 = model.predict({-2.0});
    const double true2 = 1.5 + 0.8 * -2.0 + 1.2 * std::max(0.0, -2.0 - 0.5) - 0.6 * std::max(0.0, 2.0 - 1.0);
    EXPECT_NEAR(pred2, true2, 0.2);
}

TEST(MarsRegression, BatchPredictAndTermsMetadata) {
    std::vector<std::vector<double>> X;
    std::vector<double> y;

    for (int i = 0; i <= 40; ++i) {
        const double a = -1.0 + 0.05 * static_cast<double>(i);
        const double b = 0.5 - 0.02 * static_cast<double>(i);
        const double target = 0.3 + 2.0 * std::max(0.0, a - 0.2) + 1.5 * std::max(0.0, -b - 0.1);
        X.push_back({a, b});
        y.push_back(target);
    }

    MarsRegressionFitMethod method(12, 2, 16, 1e-8);
    MarsRegression model(method);
    model.fit(X, y);

    const auto terms = model.get_terms();
    EXPECT_EQ(static_cast<int>(terms.size()), model.num_terms());
    for (const auto& term : terms) {
        EXPECT_TRUE(term.direction == 1 || term.direction == -1);
        EXPECT_TRUE(term.feature_index == 0 || term.feature_index == 1);
    }

    const auto predictions = model.predict_batch({{0.8, -0.6}, {-0.2, 0.2}});
    ASSERT_EQ(predictions.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(std::isfinite(predictions[0]));
    EXPECT_TRUE(std::isfinite(predictions[1]));
    EXPECT_GT(predictions[0], predictions[1]);
}
