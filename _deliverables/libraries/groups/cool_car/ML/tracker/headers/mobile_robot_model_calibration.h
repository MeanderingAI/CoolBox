#ifndef MOBILE_ROBOT_MODEL_CALIBRATION_H
#define MOBILE_ROBOT_MODEL_CALIBRATION_H

#include <cstddef>
#include <vector>

namespace ml {
namespace tracker {

struct RobotPose2D {
    double x = 0.0;
    double y = 0.0;
    double theta = 0.0;
};

struct OdometryControl {
    double distance = 0.0;
    double rotation = 0.0;
};

struct MotionVarianceParameters {
    double distance_from_distance = 0.0;
    double distance_from_rotation = 0.0;
    double distance_constant = 0.0;
    double turn_from_distance = 0.0;
    double turn_from_rotation = 0.0;
    double turn_constant = 0.0;
    double lateral_from_distance = 0.0;
    double lateral_from_rotation = 0.0;
    double lateral_constant = 0.0;
};

struct RangeObservation {
    double measured_range = 0.0;
    double expected_range = 0.0;
};

struct SensorMixtureParameters {
    double alpha_hit = 0.7;
    double alpha_short = 0.1;
    double alpha_max = 0.1;
    double alpha_rand = 0.1;
    double sigma_hit = 100.0;
    double lambda_short = 0.001;
    double max_range = 5000.0;
};

struct SensorResponsibilities {
    double hit = 0.0;
    double short_reading = 0.0;
    double max_reading = 0.0;
    double random = 0.0;
};

class MobileRobotModelCalibrator {
public:
    MotionVarianceParameters estimate_motion_parameters(
        const std::vector<RobotPose2D>& smoothed_trajectory,
        const std::vector<OdometryControl>& controls) const;

    SensorMixtureParameters estimate_sensor_parameters(
        const std::vector<RangeObservation>& observations,
        const SensorMixtureParameters& initial,
        std::size_t iterations = 8) const;

    std::vector<SensorResponsibilities> compute_sensor_responsibilities(
        const std::vector<RangeObservation>& observations,
        const SensorMixtureParameters& parameters) const;

    RobotPose2D apply_nominal_motion(const RobotPose2D& pose, const OdometryControl& control) const;
};

double normalize_angle(double radians);

} // namespace tracker
} // namespace ml

#endif // MOBILE_ROBOT_MODEL_CALIBRATION_H