#include "controller_learning.h"
#include "ucb_agent.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cool_car::control::learning {
class ControllerLearner::Impl {
public:
    std::size_t stepLimit;
    double exploration;
    UCBAgent agent;
    std::vector<ControllerEpisode> history;
    Impl(std::size_t steps, double coefficient) : stepLimit(steps), exploration(coefficient), agent(std::vector<double>(profileCount, 0), coefficient) {
        if (steps == 0 || steps > 100000) throw std::invalid_argument("Learning step limit must be between 1 and 100000");
    }
};
ControllerLearner::ControllerLearner(std::size_t steps, double exploration) : impl(std::make_unique<Impl>(steps, exploration)) {}
ControllerLearner::~ControllerLearner() = default;
double ControllerLearner::speedLimit(std::size_t profile) {
    if (profile >= profileCount) throw std::out_of_range("Invalid controller profile");
    return speedProfiles[profile % speedProfiles.size()];
}
bool ControllerLearner::circleRoute(std::size_t profile) {
    if (profile >= profileCount) throw std::out_of_range("Invalid controller profile");
    return profile >= speedProfiles.size();
}
std::size_t ControllerLearner::selectProfile() { return static_cast<std::size_t>(impl->agent.select_arm()); }
std::size_t ControllerLearner::bestProfile() const {
    const auto statistics = impl->agent.get_results();
    std::size_t best = 3;
    double maximum = -std::numeric_limits<double>::infinity();
    for (std::size_t profile = 0; profile < profileCount; ++profile) {
        const auto& result = statistics.bandit_results[profile];
        if (result.times_pulled > 0 && result.estimated_probability > maximum) {
            best = profile;
            maximum = result.estimated_probability;
        }
    }
    return best;
}
void ControllerLearner::observe(std::size_t profile, double reward, double seconds, int circles, bool exitReached) {
    const double minimum = -100 - timeBudget();
    if (profile >= profileCount || !std::isfinite(reward) || reward < minimum - 1 || reward > 120.000001
        || !std::isfinite(seconds) || seconds < 0 || circles < 0 || circles > 4)
        throw std::invalid_argument("Invalid exit episode return");
    if (impl->history.size() >= 100000) throw std::length_error("Controller checkpoint episode limit reached");
    const double normalized = std::clamp((reward - minimum) / (120 - minimum), 0.0, 1.0);
    impl->agent.observe_reward(static_cast<int>(profile), normalized);
    impl->history.push_back({profile, reward, seconds, circles, exitReached});
}
std::size_t ControllerLearner::episodes() const { return impl->history.size(); }
std::size_t ControllerLearner::stepLimit() const { return impl->stepLimit; }
double ControllerLearner::timeBudget() const { return impl->stepLimit * 10.0 / 120; }
std::vector<ControllerProfileStats> ControllerLearner::stats() const {
    std::vector<ControllerProfileStats> result;
    const auto statistics = impl->agent.get_results();
    for (std::size_t profile = 0; profile < profileCount; ++profile) {
        const auto& arm = statistics.bandit_results[profile];
        result.push_back({speedLimit(profile), arm.times_pulled,
            arm.times_pulled > 0 ? arm.estimated_probability * (220 + timeBudget()) - 100 - timeBudget() : 0, circleRoute(profile)});
    }
    return result;
}
const std::vector<ControllerEpisode>& ControllerLearner::history() const { return impl->history; }
double ControllerLearner::totalCost() const {
    double cost = 0;
    for (const auto& episode : impl->history) cost += episode.cost();
    return cost;
}
void ControllerLearner::save(const std::string& path) const {
    std::ofstream output(path, std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot open RL checkpoint: " + path);
    output << std::setprecision(17) << "COOLBOX_EXIT_UCB 2\n" << impl->stepLimit << ' ' << impl->exploration << '\n';
    for (double speed : speedProfiles) output << speed << ' ';
    output << '\n' << impl->history.size() << '\n';
    for (const auto& trial : impl->history)
        output << trial.profile << ' ' << trial.reward << ' ' << trial.seconds << ' ' << trial.circles << ' ' << trial.exitReached << '\n';
    output.flush();
    if (!output) throw std::runtime_error("Cannot write RL checkpoint: " + path);
}
std::unique_ptr<ControllerLearner> ControllerLearner::load(const std::string& path) {
    std::ifstream input(path);
    std::string magic;
    int version = 0;
    std::size_t steps = 0, count = 0;
    double coefficient = 0;
    if (!(input >> magic >> version >> steps >> coefficient) || magic != "COOLBOX_EXIT_UCB" || version != 2)
        throw std::runtime_error("RL checkpoint requires the circle-reward version 2; retrain into a new file: " + path);
    auto learner = std::make_unique<ControllerLearner>(steps, coefficient);
    for (double expected : speedProfiles) {
        double actual = 0;
        if (!(input >> actual) || actual != expected) throw std::runtime_error("RL checkpoint profiles do not match");
    }
    if (!(input >> count) || count > 100000) throw std::runtime_error("Invalid RL checkpoint episode count");
    for (std::size_t trial = 0; trial < count; ++trial) {
        std::size_t profile = 0;
        double reward = 0;
        double seconds = 0;
        int circles = 0, exited = 0;
        if (!(input >> profile >> reward >> seconds >> circles >> exited) || (exited != 0 && exited != 1)) throw std::runtime_error("Truncated RL checkpoint");
        learner->observe(profile, reward, seconds, circles, exited != 0);
    }
    input >> std::ws;
    if (!input.eof()) throw std::runtime_error("Unexpected RL checkpoint data");
    return learner;
}
}