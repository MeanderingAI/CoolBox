#include "tyst_framework.hpp"

#include "mobile_robot_model_calibration.h"

#include <cmath>
#include <vector>

using namespace ml::tracker;

TYST_TEST(MobileRobotModelCalibrationTest, EstimatesMotionVarianceTermsFromSmoothedTrajectory) {
    MobileRobotModelCalibrator calibrator;
    const std::vector<RobotPose2D> trajectory = {
        {0.0, 0.0, 0.0},
        {1.1, 0.1, 0.0},
        {3.3, 0.3, 0.0},
        {6.6, 0.6, 0.0},
        {11.0, 1.0, 0.0}
    };
    const std::vector<OdometryControl> controls = {
        {1.0, 0.0},
        {2.0, 0.0},
        {3.0, 0.0},
        {4.0, 0.0}
    };

    const auto parameters = calibrator.estimate_motion_parameters(trajectory, controls);

    TYST_EXPECT_TRUE(parameters.distance_from_distance > 0.005);
    TYST_EXPECT_TRUE(parameters.lateral_from_distance > 0.0005);
    TYST_EXPECT_NEAR(parameters.turn_from_distance, 0.0, 1.0e-9);
}

TYST_TEST(MobileRobotModelCalibrationTest, LearnsSonarMixtureComponents) {
    MobileRobotModelCalibrator calibrator;
    std::vector<RangeObservation> observations;
    for (int index = 0; index < 12; ++index) observations.push_back({1000.0 + static_cast<double>(index % 3 - 1) * 8.0, 1000.0});
    for (int index = 0; index < 5; ++index) observations.push_back({250.0 + static_cast<double>(index) * 20.0, 1000.0});
    for (int index = 0; index < 3; ++index) observations.push_back({5000.0, 1000.0});

    SensorMixtureParameters initial;
    initial.alpha_hit = 0.25;
    initial.alpha_short = 0.25;
    initial.alpha_max = 0.25;
    initial.alpha_rand = 0.25;
    initial.sigma_hit = 120.0;
    initial.lambda_short = 0.002;
    initial.max_range = 5000.0;

    const auto parameters = calibrator.estimate_sensor_parameters(observations, initial, 12);

    TYST_EXPECT_TRUE(parameters.alpha_hit > parameters.alpha_short);
    TYST_EXPECT_TRUE(parameters.alpha_hit > parameters.alpha_max);
    TYST_EXPECT_TRUE(parameters.alpha_short > parameters.alpha_rand);
    TYST_EXPECT_TRUE(parameters.sigma_hit < initial.sigma_hit);
}

TYST_TEST(MobileRobotModelCalibrationTest, NominalMotionUsesMajorAxisUpdate) {
    MobileRobotModelCalibrator calibrator;

    const auto next = calibrator.apply_nominal_motion({0.0, 0.0, 0.0}, {2.0, 3.14159265358979323846 / 2.0});

    TYST_EXPECT_NEAR(next.x, std::sqrt(2.0), 1.0e-9);
    TYST_EXPECT_NEAR(next.y, std::sqrt(2.0), 1.0e-9);
    TYST_EXPECT_NEAR(next.theta, 3.14159265358979323846 / 2.0, 1.0e-9);
}
