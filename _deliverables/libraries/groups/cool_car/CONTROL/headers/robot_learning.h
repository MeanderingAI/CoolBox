#ifndef COOLBOX_ROBOT_LEARNING_H
#define COOLBOX_ROBOT_LEARNING_H

#include "robot_navigation.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

namespace cool_car::control::learning {

struct Observation {
    Pose2D estimatedPose;
    std::vector<double> lidar;
    std::array<double, 2> forwardVector{};
    std::array<double, 2> goalVector{};
    double speed = 0;
    double turnRate = 0;
    double uncertainty = 0;
    double elapsed = 0;
    bool contact = false;
    bool exitReached = false;
    int circlesCollected = 0;
    int lapsCompleted = 0;
};
struct Action { double speed = 0; double turnRate = 0; };
struct ActionSpace {
    double minimumSpeed = -4;
    double maximumSpeed = 4;
    double maximumTurnRate = 1.8;
    Action constrain(Action action) const {
        if (!std::isfinite(minimumSpeed) || !std::isfinite(maximumSpeed) || minimumSpeed > maximumSpeed
            || !std::isfinite(maximumTurnRate) || maximumTurnRate < 0) throw std::invalid_argument("Invalid action-space limits");
        if (!std::isfinite(action.speed) || !std::isfinite(action.turnRate)) throw std::invalid_argument("Action must be finite");
        return {std::clamp(action.speed, minimumSpeed, maximumSpeed), std::clamp(action.turnRate, -maximumTurnRate, maximumTurnRate)};
    }
};
struct EnvironmentStep { Observation observation; bool terminated = false; bool truncated = false; };
class Environment {
public:
    virtual ~Environment() = default;
    virtual Observation reset() = 0;
    virtual EnvironmentStep step(Action action) = 0;
    virtual ActionSpace actionSpace() const { return {}; }
};
struct Transition {
    Observation before;
    Action action;
    Observation after;
    std::optional<double> reward;
    bool terminated = false;
    bool truncated = false;
};
using Policy = std::function<Action(const Observation&)>;
using RewardFunction = std::function<double(const Transition&)>;
using TransitionSink = std::function<void(const Transition&)>;

struct ExitRewardConfig {
    double timeCostPerSecond = 1;
    double exitBonus = 100;
    double failurePenalty = 100;
    double episodeTimeLimitSeconds = 120;
    double circleBonus = 5;
    double lapBonus = 20;
};

inline RewardFunction exitQuicknessReward(ExitRewardConfig config = {}) {
    if (!std::isfinite(config.timeCostPerSecond) || config.timeCostPerSecond <= 0
        || !std::isfinite(config.exitBonus) || config.exitBonus < 0
        || !std::isfinite(config.failurePenalty) || config.failurePenalty < 0
        || !std::isfinite(config.episodeTimeLimitSeconds) || config.episodeTimeLimitSeconds <= 0
        || !std::isfinite(config.circleBonus) || config.circleBonus < 0
        || !std::isfinite(config.lapBonus) || config.lapBonus < 0)
        throw std::invalid_argument("Invalid exit reward configuration");
    return [config](const Transition& transition) {
        const double elapsed = transition.after.elapsed;
        const double delta = elapsed - transition.before.elapsed;
        if (!std::isfinite(elapsed) || !std::isfinite(delta) || elapsed < 0 || delta < 0)
            throw std::invalid_argument("Episode time must be finite and nondecreasing");
        double reward = -config.timeCostPerSecond * delta;
        const int newCircles = transition.after.circlesCollected - transition.before.circlesCollected;
        if (newCircles < 0 || transition.before.circlesCollected < 0 || transition.after.circlesCollected > 4)
            throw std::invalid_argument("Circle collection must be monotonic and bounded");
        reward += config.circleBonus * newCircles;
        const int newLaps = transition.after.lapsCompleted - transition.before.lapsCompleted;
        if (newLaps < 0 || transition.before.lapsCompleted < 0)
            throw std::invalid_argument("Lap completion must be monotonic");
        reward += config.lapBonus * newLaps;
        if (transition.after.exitReached && !transition.after.contact) {
            if (!transition.before.exitReached) reward += config.exitBonus;
        } else if (transition.terminated || transition.truncated || transition.after.contact) {
            reward -= config.failurePenalty + config.timeCostPerSecond * std::max(0.0, config.episodeTimeLimitSeconds - elapsed)
                + config.circleBonus * transition.after.circlesCollected + config.lapBonus * transition.after.lapsCompleted;
        }
        return reward;
    };
}

class EpisodeRunner {
public:
    std::vector<Transition> run(Environment& environment, const Policy& policy,
                                std::size_t stepLimit, const RewardFunction& reward = {},
                                const TransitionSink& sink = {}) const {
        if (!policy || stepLimit == 0) throw std::invalid_argument("Policy and positive episode limit are required");
        auto observation = environment.reset();
        std::vector<Transition> trajectory;
        for (std::size_t step = 0; step < stepLimit; ++step) {
            const auto action = environment.actionSpace().constrain(policy(observation));
            auto result = environment.step(action);
            Transition transition{observation, action, result.observation, std::nullopt, result.terminated,
                                  result.truncated || (step + 1 == stepLimit && !result.terminated)};
            if (reward) {
                transition.reward = reward(transition);
                if (!std::isfinite(*transition.reward)) throw std::invalid_argument("Reward must be finite");
            }
            if (sink) sink(transition);
            trajectory.push_back(transition);
            observation = result.observation;
            if (transition.terminated || transition.truncated) break;
        }
        return trajectory;
    }
};
}
#endif