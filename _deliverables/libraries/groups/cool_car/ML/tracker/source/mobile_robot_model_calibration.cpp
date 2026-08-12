#include "mobile_robot_model_calibration.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace {

constexpr double pi = 3.14159265358979323846;
constexpr double min_probability = 1.0e-300;
constexpr double min_variance = 1.0e-12;

double clamp_nonnegative(double value) {
    return value > 0.0 && std::isfinite(value) ? value : 0.0;
}

std::array<double, 3> solve_three_by_three(std::array<std::array<double, 3>, 3> matrix, std::array<double, 3> rhs) {
    for (std::size_t pivot = 0; pivot < 3; ++pivot) {
        std::size_t best_row = pivot;
        for (std::size_t row = pivot + 1; row < 3; ++row) {
            if (std::abs(matrix[row][pivot]) > std::abs(matrix[best_row][pivot])) best_row = row;
        }
        if (std::abs(matrix[best_row][pivot]) < 1.0e-12) continue;
        if (best_row != pivot) {
            std::swap(matrix[best_row], matrix[pivot]);
            std::swap(rhs[best_row], rhs[pivot]);
        }
        const double scale = matrix[pivot][pivot];
        for (std::size_t column = pivot; column < 3; ++column) matrix[pivot][column] /= scale;
        rhs[pivot] /= scale;
        for (std::size_t row = 0; row < 3; ++row) {
            if (row == pivot) continue;
            const double factor = matrix[row][pivot];
            for (std::size_t column = pivot; column < 3; ++column) matrix[row][column] -= factor * matrix[pivot][column];
            rhs[row] -= factor * rhs[pivot];
        }
    }
    return {rhs[0], rhs[1], rhs[2]};
}

std::array<double, 3> fit_variance_terms(
    const std::vector<double>& squared_errors,
    const std::vector<ml::tracker::OdometryControl>& controls) {
    std::array<std::array<double, 3>, 3> normal{};
    std::array<double, 3> rhs{};
    for (std::size_t index = 0; index < squared_errors.size(); ++index) {
        const std::array<double, 3> row = {
            controls[index].distance * controls[index].distance,
            controls[index].rotation * controls[index].rotation,
            1.0
        };
        for (std::size_t outer = 0; outer < 3; ++outer) {
            rhs[outer] += row[outer] * squared_errors[index];
            for (std::size_t inner = 0; inner < 3; ++inner) normal[outer][inner] += row[outer] * row[inner];
        }
    }
    const auto solution = solve_three_by_three(normal, rhs);
    return {clamp_nonnegative(solution[0]), clamp_nonnegative(solution[1]), clamp_nonnegative(solution[2])};
}

double gaussian_hit(double measured, double expected, double sigma, double max_range) {
    if (measured < 0.0 || measured > max_range || sigma <= 0.0) return 0.0;
    const double diff = measured - expected;
    return std::exp(-0.5 * diff * diff / (sigma * sigma)) / (sigma * std::sqrt(2.0 * pi));
}

double exponential_short(double measured, double expected, double lambda) {
    if (measured < 0.0 || measured > expected || lambda <= 0.0) return 0.0;
    const double normalizer = 1.0 - std::exp(-lambda * std::max(expected, 0.0));
    if (normalizer <= 0.0) return 0.0;
    return lambda * std::exp(-lambda * measured) / normalizer;
}

double point_max(double measured, double max_range) {
    return std::abs(measured - max_range) <= 1.0e-9 ? 1.0 : 0.0;
}

double uniform_random(double measured, double max_range) {
    return measured >= 0.0 && measured <= max_range && max_range > 0.0 ? 1.0 / max_range : 0.0;
}

} // namespace

namespace ml {
namespace tracker {

double normalize_angle(double radians) {
    while (radians > pi) radians -= 2.0 * pi;
    while (radians <= -pi) radians += 2.0 * pi;
    return radians;
}

MotionVarianceParameters MobileRobotModelCalibrator::estimate_motion_parameters(
    const std::vector<RobotPose2D>& smoothed_trajectory,
    const std::vector<OdometryControl>& controls) const {
    if (smoothed_trajectory.size() != controls.size() + 1) {
        throw std::invalid_argument("Motion calibration requires one more pose than control command");
    }
    if (controls.empty()) throw std::invalid_argument("Motion calibration requires controls");

    std::vector<double> distance_errors;
    std::vector<double> turn_errors;
    std::vector<double> lateral_errors;
    distance_errors.reserve(controls.size());
    turn_errors.reserve(controls.size());
    lateral_errors.reserve(controls.size());

    for (std::size_t index = 0; index < controls.size(); ++index) {
        const RobotPose2D& previous = smoothed_trajectory[index];
        const RobotPose2D& next = smoothed_trajectory[index + 1];
        const double turn = normalize_angle(next.theta - previous.theta);
        const double dx = next.x - previous.x;
        const double dy = next.y - previous.y;
        const double major_axis = previous.theta + 0.5 * turn;
        const double projected_distance = dx * std::cos(major_axis) + dy * std::sin(major_axis);
        const double projected_lateral = -dx * std::sin(major_axis) + dy * std::cos(major_axis);
        distance_errors.push_back((projected_distance - controls[index].distance) * (projected_distance - controls[index].distance));
        turn_errors.push_back((turn - controls[index].rotation) * (turn - controls[index].rotation));
        lateral_errors.push_back(projected_lateral * projected_lateral);
    }

    const auto distance_terms = fit_variance_terms(distance_errors, controls);
    const auto turn_terms = fit_variance_terms(turn_errors, controls);
    const auto lateral_terms = fit_variance_terms(lateral_errors, controls);

    return {
        distance_terms[0], distance_terms[1], distance_terms[2],
        turn_terms[0], turn_terms[1], turn_terms[2],
        lateral_terms[0], lateral_terms[1], lateral_terms[2]
    };
}

SensorMixtureParameters MobileRobotModelCalibrator::estimate_sensor_parameters(
    const std::vector<RangeObservation>& observations,
    const SensorMixtureParameters& initial,
    std::size_t iterations) const {
    if (observations.empty()) throw std::invalid_argument("Sensor calibration requires observations");
    SensorMixtureParameters parameters = initial;
    parameters.sigma_hit = std::max(parameters.sigma_hit, 1.0e-6);
    parameters.lambda_short = std::max(parameters.lambda_short, 1.0e-9);

    for (std::size_t iteration = 0; iteration < std::max<std::size_t>(iterations, 1); ++iteration) {
        const auto responsibilities = compute_sensor_responsibilities(observations, parameters);
        double hit_sum = 0.0;
        double short_sum = 0.0;
        double max_sum = 0.0;
        double random_sum = 0.0;
        double hit_squared_error = 0.0;
        double short_weighted_distance = 0.0;
        for (std::size_t index = 0; index < observations.size(); ++index) {
            const auto& responsibility = responsibilities[index];
            const auto& observation = observations[index];
            hit_sum += responsibility.hit;
            short_sum += responsibility.short_reading;
            max_sum += responsibility.max_reading;
            random_sum += responsibility.random;
            const double diff = observation.measured_range - observation.expected_range;
            hit_squared_error += responsibility.hit * diff * diff;
            short_weighted_distance += responsibility.short_reading * observation.measured_range;
        }
        const double total = hit_sum + short_sum + max_sum + random_sum;
        parameters.alpha_hit = hit_sum / total;
        parameters.alpha_short = short_sum / total;
        parameters.alpha_max = max_sum / total;
        parameters.alpha_rand = random_sum / total;
        parameters.sigma_hit = std::sqrt(std::max(hit_squared_error / std::max(hit_sum, min_variance), min_variance));
        parameters.lambda_short = short_sum > min_variance && short_weighted_distance > min_variance
            ? short_sum / short_weighted_distance
            : parameters.lambda_short;
    }
    return parameters;
}

std::vector<SensorResponsibilities> MobileRobotModelCalibrator::compute_sensor_responsibilities(
    const std::vector<RangeObservation>& observations,
    const SensorMixtureParameters& parameters) const {
    std::vector<SensorResponsibilities> responsibilities;
    responsibilities.reserve(observations.size());
    for (const auto& observation : observations) {
        const double hit = parameters.alpha_hit * gaussian_hit(
            observation.measured_range, observation.expected_range, parameters.sigma_hit, parameters.max_range);
        const double short_reading = parameters.alpha_short * exponential_short(
            observation.measured_range, observation.expected_range, parameters.lambda_short);
        const double max_reading = parameters.alpha_max * point_max(observation.measured_range, parameters.max_range);
        const double random = parameters.alpha_rand * uniform_random(observation.measured_range, parameters.max_range);
        const double total = std::max(hit + short_reading + max_reading + random, min_probability);
        responsibilities.push_back({hit / total, short_reading / total, max_reading / total, random / total});
    }
    return responsibilities;
}

RobotPose2D MobileRobotModelCalibrator::apply_nominal_motion(const RobotPose2D& pose, const OdometryControl& control) const {
    const double major_axis = pose.theta + 0.5 * control.rotation;
    return {
        pose.x + control.distance * std::cos(major_axis),
        pose.y + control.distance * std::sin(major_axis),
        normalize_angle(pose.theta + control.rotation)
    };
}

} // namespace tracker
} // namespace ml