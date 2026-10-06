#include "robot_navigation.h"
#include "extended_kalman_filter.h"
#include "unscented_kalman_filter.h"
#include "sequential_monte_carlo.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cool_car::control {
namespace {
constexpr double robotRadius = 0.85;
constexpr double safetyMargin = 0.12;
constexpr double acceleration = 4;
double wrap(double angle) { return std::atan2(std::sin(angle), std::cos(angle)); }
double distance(Pose2D first, Pose2D second) { return std::hypot(first.x - second.x, first.z - second.z); }
mytrix::Matrix diagonal(std::size_t dimension, double value) {
    auto matrix = mytrix::Matrix::Identity(dimension);
    for (std::size_t index = 0; index < dimension; ++index) matrix.at(index, index) = value;
    return matrix;
}
}

class RobotNavigation::Impl {
public:
    std::vector<MapBox> map;
    std::vector<double> angles;
    double maximumRange;
    double sensorOffset;
    double forwardDistance = 0;
    double yawDelta = 0;
    std::vector<bool> validBeams;
    std::unique_ptr<ExtendedKalmanFilter> filter;
    std::unique_ptr<UnscentedKalmanFilter> ukf;
    mytrix::Vector ukfReference;
    std::unique_ptr<SequentialMonteCarlo> particles;
    LocalizationMode mode = LocalizationMode::ExtendedKalman;

    Impl(std::vector<MapBox> boxes, std::vector<double> beamAngles, double range, double offset)
        : map(std::move(boxes)), angles(std::move(beamAngles)), maximumRange(range), sensorOffset(offset) {
        if (angles.empty() || !std::isfinite(range) || range <= 0 || !std::isfinite(offset))
            throw std::invalid_argument("Invalid lidar configuration");
        for (double angle : angles) if (!std::isfinite(angle)) throw std::invalid_argument("Invalid beam angle");
        for (const auto& box : map) {
            if (!std::isfinite(box.minX) || !std::isfinite(box.maxX) || !std::isfinite(box.minZ) || !std::isfinite(box.maxZ)
                || box.maxX <= box.minX || box.maxZ <= box.minZ) throw std::invalid_argument("Invalid map box");
        }
    }

    double rayRange(Pose2D pose, double relativeAngle) const {
        const std::array<double, 2> origin{pose.x + std::sin(pose.yaw) * sensorOffset, pose.z + std::cos(pose.yaw) * sensorOffset};
        const std::array<double, 2> direction{std::sin(pose.yaw + relativeAngle), std::cos(pose.yaw + relativeAngle)};
        double nearest = maximumRange;
        for (const auto& box : map) {
            if (!box.lidarVisible) continue;
            const std::array<double, 2> lower{box.minX, box.minZ}, upper{box.maxX, box.maxZ};
            double nearDistance = 0, farDistance = maximumRange;
            bool intersects = true;
            for (std::size_t axis = 0; axis < 2; ++axis) {
                if (std::abs(direction[axis]) < 1e-10) {
                    if (origin[axis] < lower[axis] || origin[axis] > upper[axis]) intersects = false;
                } else {
                    double first = (lower[axis] - origin[axis]) / direction[axis];
                    double second = (upper[axis] - origin[axis]) / direction[axis];
                    if (first > second) std::swap(first, second);
                    nearDistance = std::max(nearDistance, first);
                    farDistance = std::min(farDistance, second);
                    if (nearDistance > farDistance) intersects = false;
                }
            }
            if (intersects) nearest = std::min(nearest, nearDistance);
        }
        return nearest;
    }

    mytrix::Vector measurement(const mytrix::Vector& state) const {
        auto result = mytrix::Vector::Zero(angles.size() + 1);
        const Pose2D pose{state.at(0), state.at(1), state.at(2)};
        for (std::size_t beam = 0; beam < angles.size(); ++beam) result.at(beam) = rayRange(pose, angles[beam]);
        result.at(angles.size()) = pose.yaw;
        return result;
    }

    double clearance(Pose2D pose, const std::vector<Pose2D>& hits) const {
        double result = maximumRange;
        for (const auto& box : map) {
            const double deltaX = std::max({box.minX - pose.x, 0.0, pose.x - box.maxX});
            const double deltaZ = std::max({box.minZ - pose.z, 0.0, pose.z - box.maxZ});
            result = std::min(result, std::hypot(deltaX, deltaZ));
        }
        for (const auto& hit : hits) result = std::min(result, distance(pose, hit));
        return result;
    }
};

RobotNavigation::RobotNavigation(std::vector<MapBox> map, std::vector<double> beamAngles, double range, double offset)
    : impl(std::make_unique<Impl>(std::move(map), std::move(beamAngles), range, offset)) { reset({}); }
RobotNavigation::~RobotNavigation() = default;

void RobotNavigation::reset(Pose2D initialPose) {
    if (!std::isfinite(initialPose.x) || !std::isfinite(initialPose.z) || !std::isfinite(initialPose.yaw)) throw std::invalid_argument("Invalid initial pose");
    const auto initial = mytrix::Vector(std::vector<double>{initialPose.x, initialPose.z, initialPose.yaw}, 3);
    auto covariance = diagonal(3, 0.16), processNoise = diagonal(3, 1e-6);
    covariance.at(2, 2) = 0.015;
    processNoise.at(2, 2) = 1e-7;
    auto measurementNoise = diagonal(impl->angles.size() + 1, 0.04);
    measurementNoise.at(impl->angles.size(), impl->angles.size()) = 0.0001;
    impl->filter = std::make_unique<ExtendedKalmanFilter>(initial, covariance, processNoise, measurementNoise);
    impl->ukf = std::make_unique<UnscentedKalmanFilter>(3, static_cast<int>(impl->angles.size() + 1));
    impl->ukf->initialize(initial, covariance);
    impl->ukfReference = impl->measurement(initial);
    impl->validBeams.assign(impl->angles.size(), true);
    std::mt19937 generator(1337);
    std::normal_distribution<double> positionNoise(0, 0.2), angleNoise(0, 0.04);
    std::vector<mytrix::Vector> states;
    for (int particle = 0; particle < 192; ++particle)
        states.emplace_back(std::vector<double>{initialPose.x + positionNoise(generator), initialPose.z + positionNoise(generator), initialPose.yaw + angleNoise(generator)}, 3);
    impl->particles = std::make_unique<SequentialMonteCarlo>(states, 1337);
    impl->particles->setMotionModel([](const mytrix::Vector& state, const mytrix::Vector& control, double, std::mt19937& random) {
        auto next = state;
        const double midYaw = state.at(2) + control.at(1) * 0.5;
        std::normal_distribution<double> linearNoise(0, 0.0003 + std::abs(control.at(0)) * 0.008);
        std::normal_distribution<double> turnNoise(0, 0.0002 + std::abs(control.at(1)) * 0.008);
        next.at(0) += control.at(0) * std::sin(midYaw) + linearNoise(random);
        next.at(1) += control.at(0) * std::cos(midYaw) + linearNoise(random);
        next.at(2) += control.at(1) + turnNoise(random);
        return next;
    });
    impl->particles->setLogLikelihoodModel([this](const mytrix::Vector& state, const mytrix::Vector& observation) {
        const Pose2D poseValue{state.at(0), state.at(1), state.at(2)};
        double likelihood = 0;
        for (std::size_t beam = 0; beam < impl->angles.size(); ++beam) {
            const double residual = observation.at(beam) - impl->rayRange(poseValue, impl->angles[beam]);
            if (impl->validBeams[beam]) likelihood -= std::min(residual * residual / 0.04, 25.0) * 0.5;
        }
        const double headingError = wrap(observation.at(impl->angles.size()) - state.at(2));
        return likelihood - headingError * headingError / 0.0002;
    });
    const auto processModel = [this](const mytrix::Vector& state) {
        auto next = state;
        const double midYaw = state.at(2) + impl->yawDelta * 0.5;
        next.at(0) += impl->forwardDistance * std::sin(midYaw);
        next.at(1) += impl->forwardDistance * std::cos(midYaw);
        next.at(2) += impl->yawDelta;
        return next;
    };
    impl->ukf->setProcessModel(processModel, processNoise);
    impl->filter->setProcessModel(processModel, [this](const mytrix::Vector& state) {
        auto jacobian = mytrix::Matrix::Identity(3);
        const double midYaw = state.at(2) + impl->yawDelta * 0.5;
        jacobian.at(0, 2) = impl->forwardDistance * std::cos(midYaw);
        jacobian.at(1, 2) = -impl->forwardDistance * std::sin(midYaw);
        return jacobian;
    });
    impl->filter->setMeasurementModel([this](const mytrix::Vector& state) { return impl->measurement(state); },
        [this](const mytrix::Vector& state) {
            mytrix::Matrix jacobian(impl->angles.size() + 1, 3);
            jacobian.data.setZero();
            for (std::size_t axis = 0; axis < 3; ++axis) {
                auto plus = state, minus = state;
                plus.at(axis) += 0.001;
                minus.at(axis) -= 0.001;
                const auto upper = impl->measurement(plus), lower = impl->measurement(minus);
                for (std::size_t beam = 0; beam < impl->angles.size(); ++beam) {
                    if (impl->validBeams[beam]) jacobian.at(beam, axis) = (upper.at(beam) - lower.at(beam)) / 0.002;
                }
            }
            jacobian.at(impl->angles.size(), 2) = 1;
            return jacobian;
        });
    impl->ukf->setMeasurementModel([this](const mytrix::Vector& state) {
        auto predicted = impl->measurement(state);
        for (std::size_t beam = 0; beam < impl->angles.size(); ++beam)
            if (!impl->validBeams[beam]) predicted.at(beam) = impl->ukfReference.at(beam);
        return predicted;
    }, measurementNoise);
}

void RobotNavigation::predict(double forwardDistance, double yawDelta) {
    if (!std::isfinite(forwardDistance) || !std::isfinite(yawDelta)) throw std::invalid_argument("Invalid odometry");
    impl->forwardDistance = forwardDistance;
    impl->yawDelta = yawDelta;
    impl->filter->predict();
    impl->ukf->predict();
    impl->particles->predict(mytrix::Vector(std::vector<double>{forwardDistance, yawDelta}, 2), 1);
}

void RobotNavigation::observe(const std::vector<double>& ranges, std::array<double, 2> forwardVector) {
    if (ranges.size() != impl->angles.size()) throw std::invalid_argument("Lidar beam count mismatch");
    if (!std::isfinite(forwardVector[0]) || !std::isfinite(forwardVector[1]) || std::hypot(forwardVector[0], forwardVector[1]) < 1e-9)
        throw std::invalid_argument("Invalid forward vector");
    auto measurement = impl->measurement(impl->filter->state());
    for (std::size_t beam = 0; beam < ranges.size(); ++beam) {
        impl->validBeams[beam] = std::isfinite(ranges[beam]) && ranges[beam] >= 0 && ranges[beam] <= impl->maximumRange
            && std::abs(ranges[beam] - measurement.at(beam)) < 1.5;
        if (impl->validBeams[beam]) measurement.at(beam) = ranges[beam];
    }
    const auto headingIndex = impl->angles.size();
    measurement.at(headingIndex) += wrap(std::atan2(forwardVector[0], forwardVector[1]) - measurement.at(headingIndex));
    impl->filter->update(measurement);
    impl->particles->update(measurement);
    impl->ukfReference = impl->measurement(impl->ukf->state());
    auto ukfMeasurement = impl->ukfReference;
    for (std::size_t beam = 0; beam < ranges.size(); ++beam)
        if (impl->validBeams[beam]) ukfMeasurement.at(beam) = ranges[beam];
    ukfMeasurement.at(headingIndex) += wrap(std::atan2(forwardVector[0], forwardVector[1]) - ukfMeasurement.at(headingIndex));
    impl->ukf->update(ukfMeasurement);
}

void RobotNavigation::setLocalizationMode(LocalizationMode mode) { impl->mode = mode; }
Pose2D RobotNavigation::pose() const {
    if (impl->mode == LocalizationMode::ParticleFilter) {
        Pose2D estimate;
        double sine = 0, cosine = 0;
        for (const auto& particle : impl->particles->getParticles()) {
            estimate.x += particle.state.at(0) * particle.weight;
            estimate.z += particle.state.at(1) * particle.weight;
            sine += std::sin(particle.state.at(2)) * particle.weight;
            cosine += std::cos(particle.state.at(2)) * particle.weight;
        }
        estimate.yaw = std::atan2(sine, cosine);
        return estimate;
    }
    const auto& state = impl->mode == LocalizationMode::UnscentedKalman ? impl->ukf->state() : impl->filter->state();
    return {state.at(0), state.at(1), wrap(state.at(2))};
}
double RobotNavigation::positionUncertainty() const {
    if (impl->mode == LocalizationMode::ParticleFilter) {
        const auto estimate = pose();
        double variance = 0;
        for (const auto& particle : impl->particles->getParticles()) {
            const double deltaX = particle.state.at(0) - estimate.x, deltaZ = particle.state.at(1) - estimate.z;
            variance += (deltaX * deltaX + deltaZ * deltaZ) * particle.weight;
        }
        return std::sqrt(variance);
    }
    const auto& covariance = impl->mode == LocalizationMode::UnscentedKalman ? impl->ukf->covariance() : impl->filter->covariance();
    return std::sqrt(std::max(0.0, covariance.at(0, 0) + covariance.at(1, 1)));
}
std::vector<Pose2D> RobotNavigation::particlePoses() const {
    std::vector<Pose2D> result;
    for (const auto& particle : impl->particles->getParticles()) result.push_back({particle.state.at(0), particle.state.at(1), wrap(particle.state.at(2))});
    return result;
}
std::vector<double> RobotNavigation::expectedRanges(Pose2D poseValue) const {
    std::vector<double> ranges;
    for (double angle : impl->angles) ranges.push_back(impl->rayRange(poseValue, angle));
    return ranges;
}

DrivePlan RobotNavigation::plan(Pose2D goal, const std::vector<double>& ranges, double currentSpeed, double currentTurnRate, double speedLimit) const {
    DrivePlan best;
    best.freeDirections.assign(impl->angles.size(), false);
    if (ranges.size() != impl->angles.size()) throw std::invalid_argument("Lidar beam count mismatch");
    if (!std::isfinite(goal.x) || !std::isfinite(goal.z) || !std::isfinite(currentSpeed) || !std::isfinite(currentTurnRate)
        || !std::isfinite(speedLimit) || speedLimit <= 0) return best;
    for (double range : ranges) if (!std::isfinite(range) || range < 0 || range > impl->maximumRange) return best;
    const auto initial = pose();
    std::vector<Pose2D> hits;
    const double brakingDistance = currentSpeed * currentSpeed / (2 * acceleration);
    for (std::size_t beam = 0; beam < ranges.size(); ++beam) {
        best.freeDirections[beam] = ranges[beam] > robotRadius + safetyMargin + brakingDistance;
        if (ranges[beam] < impl->maximumRange - 0.01) {
            hits.push_back({initial.x + std::sin(initial.yaw) * impl->sensorOffset + std::sin(initial.yaw + impl->angles[beam]) * ranges[beam],
                initial.z + std::cos(initial.yaw) * impl->sensorOffset + std::cos(initial.yaw + impl->angles[beam]) * ranges[beam], 0});
        }
    }
    if (distance(initial, goal) < 0.15) { best.blocked = false; return best; }
    const double lowerSpeed = std::max(0.0, currentSpeed - acceleration * 0.15);
    const double upperSpeed = std::clamp(currentSpeed + acceleration * 0.15, 0.0, std::min(speedLimit, 4.0));
    const double lowerTurn = std::clamp(currentTurnRate - 1.0, -1.8, 1.8);
    const double upperTurn = std::clamp(currentTurnRate + 1.0, -1.8, 1.8);
    double bestCost = std::numeric_limits<double>::infinity();
    bool safeForward = false;
    for (int speedSample = 0; speedSample <= 5; ++speedSample) {
        const double speed = speedSample == 0 ? 0 : std::max(0.0, std::min(lowerSpeed, upperSpeed) + (upperSpeed - std::min(lowerSpeed, upperSpeed)) * speedSample / 5);
        for (int turnSample = 0; turnSample <= 10; ++turnSample) {
            const double turn = lowerTurn + (upperTurn - lowerTurn) * turnSample / 10;
            auto predicted = initial;
            double nearest = impl->clearance(predicted, hits);
            bool safe = nearest >= robotRadius + safetyMargin;
            std::vector<Pose2D> path;
            for (int step = 0; step < 15 && safe; ++step) {
                const double midYaw = predicted.yaw + turn * 0.05;
                predicted.x += speed * std::sin(midYaw) * 0.1;
                predicted.z += speed * std::cos(midYaw) * 0.1;
                predicted.yaw += turn * 0.1;
                nearest = std::min(nearest, impl->clearance(predicted, hits));
                safe = nearest >= robotRadius + safetyMargin + speed * speed / (2 * acceleration);
                path.push_back(predicted);
            }
            if (!safe) continue;
            if (speed > 0.05) safeForward = true;
            const double headingError = std::abs(wrap(std::atan2(goal.x - predicted.x, goal.z - predicted.z) - predicted.yaw));
            const double cost = 2.8 * headingError + 1.4 * distance(predicted, goal) - 0.65 * speed
                + 0.25 / (0.1 + nearest) + 0.08 * std::abs(turn - currentTurnRate);
            if (cost < bestCost) {
                bestCost = cost;
                best.speed = speed;
                best.turnRate = turn;
                best.clearance = nearest - robotRadius;
                best.path = std::move(path);
            }
        }
    }
    best.blocked = !safeForward;
    return best;
}
}