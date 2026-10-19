#ifndef COOLBOX_ROBOT_NAVIGATION_H
#define COOLBOX_ROBOT_NAVIGATION_H

#include <array>
#include <memory>
#include <vector>

namespace cool_car::control {

struct Pose2D {
    double x = 0;
    double z = 0;
    double yaw = 0;
};

struct MapBox {
    double minX;
    double minZ;
    double maxX;
    double maxZ;
    bool lidarVisible = true;
};

struct DrivePlan {
    double speed = 0;
    double turnRate = 0;
    bool blocked = true;
    double clearance = 0;
    std::vector<Pose2D> path;
    std::vector<bool> freeDirections;
};

enum class LocalizationMode { ExtendedKalman, UnscentedKalman, ParticleFilter };

class RobotNavigation {
public:
    RobotNavigation(std::vector<MapBox> map, std::vector<double> beamAngles,
                    double sensorRange = 8, double sensorForwardOffset = 0.66);
    ~RobotNavigation();
    RobotNavigation(const RobotNavigation&) = delete;
    RobotNavigation& operator=(const RobotNavigation&) = delete;

    void reset(Pose2D initialPose);
    void predict(double forwardDistance, double yawDelta);
    void observe(const std::vector<double>& ranges, std::array<double, 2> forwardVector);
    void setLocalizationMode(LocalizationMode mode);
    Pose2D pose() const;
    std::vector<Pose2D> particlePoses() const;
    double positionUncertainty() const;
    std::vector<double> expectedRanges(Pose2D pose) const;
    DrivePlan plan(Pose2D goal, const std::vector<double>& ranges,
                   double currentSpeed, double currentTurnRate, double speedLimit) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

}
#endif