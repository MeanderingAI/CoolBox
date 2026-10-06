#ifndef COOLBOX_CONTROLLER_LEARNING_H
#define COOLBOX_CONTROLLER_LEARNING_H

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace cool_car::control::learning {

struct ControllerProfileStats {
    double speedLimit;
    int episodes;
    double meanReturn;
    bool circleRoute = false;
};

struct ControllerEpisode {
    std::size_t profile;
    double reward;
    double seconds;
    int circles;
    bool exitReached;
    double cost() const { return exitReached ? 100 - reward : -reward; }
};

class ControllerLearner {
public:
    inline static constexpr std::array<double, 6> speedProfiles{0.8, 1.2, 1.8, 2.4, 3.2, 4.0};
    inline static constexpr std::size_t profileCount = 12;
    static double speedLimit(std::size_t profile);
    static bool circleRoute(std::size_t profile);
    explicit ControllerLearner(std::size_t stepLimit = 1000, double exploration = 0.12);
    ~ControllerLearner();
    ControllerLearner(const ControllerLearner&) = delete;
    ControllerLearner& operator=(const ControllerLearner&) = delete;
    std::size_t selectProfile();
    std::size_t bestProfile() const;
    void observe(std::size_t profile, double episodeReturn, double seconds = 0, int circles = 0, bool exitReached = false);
    std::size_t episodes() const;
    std::size_t stepLimit() const;
    double timeBudget() const;
    std::vector<ControllerProfileStats> stats() const;
    const std::vector<ControllerEpisode>& history() const;
    double totalCost() const;
    void save(const std::string& path) const;
    static std::unique_ptr<ControllerLearner> load(const std::string& path);
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
}
#endif