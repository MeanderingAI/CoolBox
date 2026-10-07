#include <raylib.h>
#include <raymath.h>
#include <btBulletDynamicsCommon.h>
#include "robot_navigation.h"
#include "robot_learning.h"
#include "controller_learning.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

namespace {

constexpr float pi = 3.14159265358979323846f;
constexpr float timeStep = 1.0f / 120.0f;
constexpr float trackWidth = 0.96f;
constexpr float sensorRange = 8.0f;
constexpr int circlePoints = 5;
constexpr int lapPoints = 20;
constexpr Color ink{30, 42, 42, 255};
constexpr Color muted{101, 116, 113, 255};
constexpr Color paper{247, 249, 247, 255};
constexpr Color accent{19, 136, 115, 255};
constexpr Color coral{203, 100, 80, 255};
constexpr Color lineColor{218, 225, 220, 255};

struct Obstacle { Vector3 position; Vector3 size; };
const std::array<Obstacle, 8> obstacles{{
    {{-3, 0.65f, -2}, {2.3f, 1.3f, 1.6f}}, {{3, 0.45f, 2}, {2, 0.9f, 2}},
    {{-2, 0.85f, 4}, {1.4f, 1.7f, 2.4f}}, {{4, 0.6f, -4}, {2.4f, 1.2f, 1.4f}},
    {{0, 0.35f, -5}, {1.3f, 0.7f, 1.3f}}, {{-5, 0.45f, 1}, {1, 0.9f, 1}},
    {{5, 0.45f, 5}, {1, 0.9f, 1}}, {{1, 0.65f, -0.7f}, {1.1f, 1.3f, 1.1f}}
}};
const std::array<Vector3, 4> waypoints{{{-7, 0, 7}, {7, 0, 7}, {7, 0, -7}, {-7, 0, -7}}};
constexpr Vector3 exitTarget{-7, 0, 13.5f};
constexpr Vector3 figTreePosition{-2.8f, 0, 15.8f};
const std::array<Obstacle, 5> exitWalls{{
    {{-10.25f, 0.5f, 12}, {3.5f, 1, 0.4f}},
    {{3.25f, 0.5f, 12}, {17.5f, 1, 0.4f}},
    {{-8.5f, 0.5f, 13.5f}, {0.4f, 1, 3}},
    {{-5.5f, 0.5f, 13.5f}, {0.4f, 1, 3}},
    {{-7, 0.5f, 15}, {3, 1, 0.4f}}
}};

using cool_car::control::LocalizationMode;
using cool_car::control::Pose2D;

LocalizationMode parseLocalizer(const std::string& value) {
    if (value == "ekf") return LocalizationMode::ExtendedKalman;
    if (value == "ukf") return LocalizationMode::UnscentedKalman;
    if (value == "particle") return LocalizationMode::ParticleFilter;
    throw std::invalid_argument("Localizer must be ekf, ukf, or particle");
}

std::vector<double> lidarAngles() {
    std::vector<double> result;
    for (int beam = -5; beam <= 5; ++beam) result.push_back(beam * 16.0 * pi / 180);
    return result;
}
std::vector<cool_car::control::MapBox> navigationMap() {
    std::vector<cool_car::control::MapBox> result;
    for (const auto& obstacle : obstacles) {
        result.push_back({obstacle.position.x - obstacle.size.x / 2, obstacle.position.z - obstacle.size.z / 2,
            obstacle.position.x + obstacle.size.x / 2, obstacle.position.z + obstacle.size.z / 2, obstacle.size.y >= 0.76f});
    }
    result.push_back({-12.2, -12, -11.8, 12});
    result.push_back({11.8, -12, 12.2, 12});
    result.push_back({-12, -12.2, 12, -11.8});
    for (const auto& wall : exitWalls)
        result.push_back({wall.position.x - wall.size.x / 2, wall.position.z - wall.size.z / 2,
            wall.position.x + wall.size.x / 2, wall.position.z + wall.size.z / 2});
    result.push_back({figTreePosition.x - 0.5, figTreePosition.z - 0.5, figTreePosition.x + 0.5, figTreePosition.z + 0.5});
    return result;
}

Vector3 vectorFrom(const btVector3& value) {
    return {static_cast<float>(value.x()), static_cast<float>(value.y()), static_cast<float>(value.z())};
}
float wrapAngle(float angle) { return std::atan2(std::sin(angle), std::cos(angle)); }
float approach(float value, float target, float delta) { return value + std::clamp(target - value, -delta, delta); }
float manualTurnCommand(bool steerLeft, bool steerRight) {
    return (static_cast<int>(steerLeft) - static_cast<int>(steerRight)) * 1.8f;
}

struct Beam { Vector3 start{}; Vector3 end{}; float distance = sensorRange; bool hit = false; };
struct RayCallback : btCollisionWorld::ClosestRayResultCallback {
    const btCollisionObject* ignored;
    RayCallback(const btVector3& start, const btVector3& end, const btCollisionObject* robot)
        : btCollisionWorld::ClosestRayResultCallback(start, end), ignored(robot) {}
    bool needsCollision(btBroadphaseProxy* proxy) const override {
        return proxy->m_clientObject != ignored && btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy);
    }
};

class Simulation {
public:
    Simulation() : dispatcher(&configuration), world(&dispatcher, &broadphase, &solver, &configuration) {
        world.setGravity({0, -9.81f, 0});
        addBox({0, -0.25f, 0}, {24.4f, 0.5f, 24.4f}, 0);
        addBox({-5, -0.25f, 15}, {14, 0.5f, 8}, 0);
        for (const auto& obstacle : obstacles) addBox(obstacle.position, obstacle.size, 0);
        addBox({-12, 0.5f, 0}, {0.4f, 1, 24}, 0);
        addBox({12, 0.5f, 0}, {0.4f, 1, 24}, 0);
        addBox({0, 0.5f, -12}, {24, 1, 0.4f}, 0);
        for (const auto& wall : exitWalls) addBox(wall.position, wall.size, 0);
        addBox({figTreePosition.x, 1.8f, figTreePosition.z}, {1, 3.6f, 1}, 0);
        robot = addBox({-7, 0.42f, -7}, {0.86f, 0.34f, 1.22f}, 12);
        robot->setLinearFactor({1, 0, 1});
        robot->setAngularFactor({0, 1, 0});
        robot->setGravity({0, 0, 0});
        robot->setActivationState(DISABLE_DEACTIVATION);
        robot->setCcdMotionThreshold(0.1f);
        robot->setCcdSweptSphereRadius(0.3f);
        reset();
    }
    ~Simulation() { for (auto& body : bodies) world.removeRigidBody(body.get()); }
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Vector3 position() const { return vectorFrom(robot->getWorldTransform().getOrigin()); }
    float heading() const {
        const auto forward = robot->getWorldTransform().getBasis().getColumn(2);
        return std::atan2(static_cast<float>(forward.x()), static_cast<float>(forward.z()));
    }
    Vector3 localPoint(Vector3 value) const {
        return vectorFrom(robot->getWorldTransform() * btVector3(value.x, value.y, value.z));
    }
    std::vector<double> ranges() const {
        std::vector<double> result;
        for (const auto& beam : beams) result.push_back(beam.distance);
        return result;
    }
    double localizationError() const {
        const auto estimate = navigation.pose();
        const auto truth = position();
        return std::hypot(estimate.x - truth.x, estimate.z - truth.z);
    }
    bool exitReached() const {
        const auto truth = position();
        const auto& basis = robot->getWorldTransform().getBasis();
        const float rearExtent = std::abs(static_cast<float>(basis[2][0])) * 0.43f + std::abs(static_cast<float>(basis[2][2])) * 0.61f;
        return truth.x >= -8 && truth.x <= -6 && truth.z >= 12.9f && truth.z <= 14.5f && truth.z - rearExtent >= 12.2f;
    }
    void setExitMission(bool enabled) { exitMission = enabled; lastPlanningTick = -1; }
    void setCircleRoute(bool enabled) { circleRouteEnabled = enabled; lastPlanningTick = -1; }
    void setLocalizationMode(LocalizationMode mode) {
        localizationMode = mode;
        navigation.setLocalizationMode(mode);
        lastPlanningTick = -1;
    }
    void setPose(Vector3 value, float yaw) {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin({value.x, value.y, value.z});
        transform.setRotation(btQuaternion(btVector3(0, 1, 0), yaw));
        robot->setWorldTransform(transform);
        robot->setInterpolationWorldTransform(transform);
        robot->getMotionState()->setWorldTransform(transform);
        robot->setLinearVelocity({0, 0, 0});
        robot->setAngularVelocity({0, 0, 0});
        robot->clearForces();
        world.getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(robot->getBroadphaseHandle(), &dispatcher);
        world.updateSingleAabb(robot);
        leftSpeed = rightSpeed = 0;
        scan();
        navigation.reset({value.x + 0.12, value.z - 0.09, yaw + 0.02});
        navigation.setLocalizationMode(localizationMode);
        navigation.observe(ranges(), {std::sin(yaw), std::cos(yaw)});
        controlTicks = 0;
        lastPlanningTick = -1;
        drivePlan = {};
    }
    void reset() {
        setPose({-7, 0.42f, -7}, 0);
        elapsed = distanceTravelled = leftTravel = rightTravel = 0;
        collisionCount = laps = 0;
        collectedCircles.fill(false);
        circleArmed.fill(false);
        circlesCollected = 0;
        waypointIndex = 0;
        touching = wasTouching = false;
        trail.clear();
        trail.push_back(position());
    }
    std::pair<float, float> autopilot(float maximumSpeed) {
        const auto current = navigation.pose();
        auto goal = exitMission ? exitTarget : waypoints[waypointIndex];
        if (exitMission && circleRouteEnabled) {
            for (std::size_t circle = 0; circle < waypoints.size(); ++circle) {
                if (!collectedCircles[circle]) { goal = waypoints[circle]; break; }
            }
        }
        if (exitMission && exitReached()) return {0.0f, 0.0f};
        if (!exitMission && std::hypot(current.x - goal.x, current.z - goal.z) < 0.9) {
            waypointIndex = (waypointIndex + 1) % waypoints.size();
            if (waypointIndex == 0) ++laps;
            goal = waypoints[waypointIndex];
            lastPlanningTick = -1;
        }
        if (lastPlanningTick < 0 || controlTicks - lastPlanningTick >= 10 || maximumSpeed != previousSpeedLimit) {
            drivePlan = navigation.plan({goal.x, goal.z, 0}, ranges(), (leftSpeed + rightSpeed) * 0.5,
                (leftSpeed - rightSpeed) / trackWidth, maximumSpeed);
            lastPlanningTick = controlTicks;
            previousSpeedLimit = maximumSpeed;
        }
        return {static_cast<float>(drivePlan.speed), static_cast<float>(drivePlan.turnRate)};
    }
    void step(float forwardSpeed, float turnRate) {
        leftSpeed = approach(leftSpeed, forwardSpeed + turnRate * trackWidth * 0.5f, timeStep * 4);
        rightSpeed = approach(rightSpeed, forwardSpeed - turnRate * trackWidth * 0.5f, timeStep * 4);
        const float yaw = heading(), speed = (leftSpeed + rightSpeed) * 0.5f;
        const auto before = position();
        robot->setLinearVelocity({std::sin(yaw) * speed, 0, std::cos(yaw) * speed});
        robot->setAngularVelocity({0, (leftSpeed - rightSpeed) / trackWidth, 0});
        world.stepSimulation(timeStep, 0);
        for (std::size_t circle = 0; circle < waypoints.size(); ++circle) {
            const auto point = robot->getWorldTransform().inverse() * btVector3(waypoints[circle].x, position().y, waypoints[circle].z);
            const double deltaX = std::max(0.0, std::abs(static_cast<double>(point.x())) - 0.43);
            const double deltaZ = std::max(0.0, std::abs(static_cast<double>(point.z())) - 0.61);
            const bool overlapsCircle = std::hypot(deltaX, deltaZ) <= 0.45;
            if (!overlapsCircle) circleArmed[circle] = true;
            if (!collectedCircles[circle] && circleArmed[circle] && overlapsCircle) {
                collectedCircles[circle] = true;
                ++circlesCollected;
                lastPlanningTick = -1;
            }
        }
        if (exitMission && laps == 0 && circlesCollected == static_cast<int>(waypoints.size())) ++laps;
        const auto after = position();
        const float yawDelta = wrapAngle(heading() - yaw);
        const float midYaw = yaw + yawDelta * 0.5f;
        const double forwardDistance = (after.x - before.x) * std::sin(midYaw) + (after.z - before.z) * std::cos(midYaw);
        navigation.predict(forwardDistance, yawDelta);
        elapsed += timeStep;
        distanceTravelled += Vector3Distance(before, after);
        leftTravel += leftSpeed * timeStep;
        rightTravel += rightSpeed * timeStep;
        touching = false;
        for (int manifoldIndex = 0; manifoldIndex < dispatcher.getNumManifolds(); ++manifoldIndex) {
            auto* manifold = dispatcher.getManifoldByIndexInternal(manifoldIndex);
            if (manifold->getBody0() != robot && manifold->getBody1() != robot) continue;
            for (int contactIndex = 0; contactIndex < manifold->getNumContacts(); ++contactIndex) {
                if (manifold->getContactPoint(contactIndex).getDistance() <= 0.02f) touching = true;
            }
        }
        if (touching && !wasTouching) ++collisionCount;
        wasTouching = touching;
        scan();
        ++controlTicks;
        if (controlTicks % 10 == 0) navigation.observe(ranges(), {std::sin(heading()), std::cos(heading())});
        if (Vector3Distance(trail.back(), position()) > 0.12f) {
            trail.push_back(position());
            if (trail.size() > 1200) trail.pop_front();
        }
    }
    std::array<Beam, 11> beams{};
    cool_car::control::RobotNavigation navigation{navigationMap(), lidarAngles()};
    cool_car::control::DrivePlan drivePlan;
    LocalizationMode localizationMode = LocalizationMode::ExtendedKalman;
    std::deque<Vector3> trail;
    float leftSpeed = 0, rightSpeed = 0, leftTravel = 0, rightTravel = 0;
    float elapsed = 0, distanceTravelled = 0;
    int collisionCount = 0, laps = 0;
    std::size_t waypointIndex = 0;
    bool touching = false;
    bool exitMission = false;
    bool circleRouteEnabled = false;
    std::array<bool, 4> collectedCircles{};
    std::array<bool, 4> circleArmed{};
    int circlesCollected = 0;
private:
    btRigidBody* addBox(Vector3 positionValue, Vector3 size, float mass) {
        auto shape = std::make_unique<btBoxShape>(btVector3(size.x / 2, size.y / 2, size.z / 2));
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin({positionValue.x, positionValue.y, positionValue.z});
        auto motion = std::make_unique<btDefaultMotionState>(transform);
        btVector3 inertia(0, 0, 0);
        if (mass > 0) shape->calculateLocalInertia(mass, inertia);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motion.get(), shape.get(), inertia);
        auto body = std::make_unique<btRigidBody>(info);
        body->setFriction(0.6f);
        auto* result = body.get();
        world.addRigidBody(result);
        shapes.push_back(std::move(shape));
        motions.push_back(std::move(motion));
        bodies.push_back(std::move(body));
        return result;
    }
    void scan() {
        const auto start = localPoint({0, 0.34f, 0.66f});
        for (std::size_t beamIndex = 0; beamIndex < beams.size(); ++beamIndex) {
            const float angle = heading() + (static_cast<float>(beamIndex) - 5) * 16 * pi / 180;
            const Vector3 end{start.x + std::sin(angle) * sensorRange, start.y, start.z + std::cos(angle) * sensorRange};
            const btVector3 from(start.x, start.y, start.z), to(end.x, end.y, end.z);
            RayCallback callback(from, to, robot);
            world.rayTest(from, to, callback);
            beams[beamIndex] = {start, callback.hasHit() ? vectorFrom(callback.m_hitPointWorld) : end,
                callback.hasHit() ? sensorRange * static_cast<float>(callback.m_closestHitFraction) : sensorRange, callback.hasHit()};
        }
    }
    btDefaultCollisionConfiguration configuration;
    btCollisionDispatcher dispatcher;
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world;
    std::vector<std::unique_ptr<btCollisionShape>> shapes;
    std::vector<std::unique_ptr<btDefaultMotionState>> motions;
    std::vector<std::unique_ptr<btRigidBody>> bodies;
    btRigidBody* robot = nullptr;
    bool wasTouching = false;
    int controlTicks = 0;
    int lastPlanningTick = -1;
    float previousSpeedLimit = 0;
};

class RobotEnvironment : public cool_car::control::learning::Environment {
public:
    explicit RobotEnvironment(LocalizationMode mode = LocalizationMode::ExtendedKalman, float speedLimit = 2.4f, bool circleRoute = false)
        : mode(mode), speedLimit(speedLimit), circleRoute(circleRoute) {
        if (!std::isfinite(speedLimit) || speedLimit <= 0 || speedLimit > 4) throw std::invalid_argument("Speed limit must be greater than zero and at most 4 m/s");
    }
    cool_car::control::learning::Observation reset() override {
        simulation.reset();
        simulation.setLocalizationMode(mode);
        simulation.setExitMission(true);
        simulation.setCircleRoute(circleRoute);
        return observation();
    }
    cool_car::control::learning::EnvironmentStep step(cool_car::control::learning::Action action) override {
        const auto bounded = actionSpace().constrain(action);
        for (int tick = 0; tick < 10; ++tick) {
            simulation.step(static_cast<float>(bounded.speed), static_cast<float>(bounded.turnRate));
            if (simulation.collisionCount > 0 || simulation.exitReached()) break;
        }
        return {observation(), simulation.collisionCount > 0 || simulation.exitReached(), false};
    }
    cool_car::control::learning::ActionSpace actionSpace() const override { return {-speedLimit, speedLimit, 1.8}; }
    cool_car::control::learning::Action controllerAction() {
        const auto command = simulation.autopilot(speedLimit);
        return {command.first, command.second};
    }
    Simulation simulation;
private:
    cool_car::control::learning::Observation observation() const {
        const auto pose = simulation.navigation.pose();
        const auto goal = exitTarget;
        return {pose, simulation.ranges(), {std::sin(simulation.heading()), std::cos(simulation.heading())},
            {goal.x - pose.x, goal.z - pose.z}, (simulation.leftSpeed + simulation.rightSpeed) * 0.5,
            (simulation.leftSpeed - simulation.rightSpeed) / trackWidth, simulation.navigation.positionUncertainty(), simulation.elapsed,
            simulation.collisionCount > 0, simulation.exitReached(), simulation.circlesCollected, simulation.laps};
    }
    LocalizationMode mode;
    float speedLimit;
    bool circleRoute;
};

std::vector<cool_car::control::learning::Transition> collectEpisode(RobotEnvironment& environment, std::size_t stepLimit) {
    cool_car::control::learning::ExitRewardConfig config;
    config.episodeTimeLimitSeconds = stepLimit * 10.0 / 120.0;
    return cool_car::control::learning::EpisodeRunner{}.run(environment,
        [&environment](const cool_car::control::learning::Observation&) { return environment.controllerAction(); }, stepLimit,
        cool_car::control::learning::exitQuicknessReward(config));
}

double episodeReturn(const std::vector<cool_car::control::learning::Transition>& trajectory) {
    double reward = 0;
    for (const auto& transition : trajectory) {
        if (!transition.reward) throw std::runtime_error("RL training requires episode rewards");
        reward += *transition.reward;
    }
    return reward;
}

void saveLearner(const cool_car::control::learning::ControllerLearner& learner, const std::string& path) {
    const auto directory = std::filesystem::path(path).parent_path();
    if (!directory.empty()) std::filesystem::create_directories(directory);
    learner.save(path);
}

int trainController(int argc, char** argv) {
    using cool_car::control::learning::ControllerLearner;
    std::size_t episodes = 18, steps = 1000;
    std::string modelPath = "build/robot-exit-circle-policy.txt";
    LocalizationMode mode = LocalizationMode::ParticleFilter;
    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (argument + 1 >= argc) throw std::invalid_argument("Training option needs a value");
        const std::string value = argv[++argument];
        if (option == "--episodes" || option == "--steps") {
            if (value.empty() || value.front() == '-') throw std::invalid_argument("Training counts must be positive");
            std::size_t consumed = 0;
            const auto count = std::stoul(value, &consumed);
            if (consumed != value.size() || count == 0 || count > (option == "--episodes" ? 1000u : 100000u))
                throw std::invalid_argument("Training count is outside its supported range");
            if (option == "--episodes") episodes = count; else steps = count;
        } else if (option == "--model") modelPath = value;
        else if (option == "--localizer") {
            mode = parseLocalizer(value);
        } else throw std::invalid_argument("Unknown training option: " + option);
    }
    auto learner = std::filesystem::exists(modelPath) ? ControllerLearner::load(modelPath) : std::make_unique<ControllerLearner>(steps);
    if (learner->stepLimit() != steps) throw std::invalid_argument("Training step limit does not match checkpoint");
    for (std::size_t episode = 0; episode < episodes; ++episode) {
        const auto profile = learner->selectProfile();
        RobotEnvironment environment(mode, static_cast<float>(ControllerLearner::speedLimit(profile)), ControllerLearner::circleRoute(profile));
        const auto trajectory = collectEpisode(environment, steps);
        const double reward = episodeReturn(trajectory);
        learner->observe(profile, reward, environment.simulation.elapsed, environment.simulation.circlesCollected,
            environment.simulation.exitReached(), environment.simulation.laps);
        saveLearner(*learner, modelPath);
        std::cout << "training_episode=" << learner->episodes() << " speed=" << ControllerLearner::speedLimit(profile)
            << " route=" << (ControllerLearner::circleRoute(profile) ? "circles" : "direct")
            << " exit=" << environment.simulation.exitReached() << " contacts=" << environment.simulation.collisionCount
            << " seconds=" << environment.simulation.elapsed << " circles=" << environment.simulation.circlesCollected
            << " laps=" << environment.simulation.laps
            << " cost=" << learner->history().back().cost() << " return=" << reward << " total_cost=" << learner->totalCost() << '\n';
    }
    auto restored = ControllerLearner::load(modelPath);
    if (restored->bestProfile() != learner->bestProfile() || restored->episodes() != learner->episodes())
        throw std::runtime_error("RL checkpoint round-trip failed");
    const auto selected = restored->bestProfile();
    RobotEnvironment learned(mode, static_cast<float>(ControllerLearner::speedLimit(selected)), ControllerLearner::circleRoute(selected)), baseline(mode, 2.4f);
    const auto learnedTrajectory = collectEpisode(learned, steps);
    const auto baselineTrajectory = collectEpisode(baseline, steps);
    std::cout << "RL_EVALUATION speed=" << ControllerLearner::speedLimit(selected) << " exit=" << learned.simulation.exitReached()
        << " contacts=" << learned.simulation.collisionCount << " seconds=" << learned.simulation.elapsed
        << " return=" << episodeReturn(learnedTrajectory) << " baseline_return=" << episodeReturn(baselineTrajectory)
        << " checkpoint=" << modelPath << '\n';
    return learned.simulation.exitReached() && learned.simulation.collisionCount == 0 ? 0 : 1;
}

void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int diagnose() {
    using cool_car::control::learning::ControllerLearner;
    ControllerLearner learner(1000, 0);
    for (std::size_t profile = 0; profile < ControllerLearner::profileCount; ++profile) {
        check(learner.selectProfile() == profile, "UCB did not explore unobserved profiles");
        learner.observe(profile, 70 + static_cast<double>(profile));
    }
    check(learner.bestProfile() == 11 && learner.selectProfile() == 11, "UCB did not learn from external returns");
    using cool_car::control::learning::Transition;
    const auto reward = cool_car::control::learning::exitQuicknessReward();
    Transition fast{}, slow{}, collision{}, timeout{};
    fast.after.elapsed = 2;
    fast.after.exitReached = fast.terminated = true;
    slow = fast; slow.after.elapsed = 5;
    collision.after.elapsed = 2; collision.after.contact = collision.terminated = true;
    timeout.after.elapsed = 120; timeout.truncated = true;
    check(reward(fast) > reward(slow), "Faster exits must earn higher return");
    check(std::abs(reward(collision) - reward(timeout)) < 1e-9, "Collision exploited the time budget");
    Transition collectible{}, revisit{};
    collectible.after.elapsed = 1; collectible.after.circlesCollected = 1;
    revisit.before = collectible.after; revisit.after = revisit.before; revisit.after.elapsed = 2;
    check(std::abs(reward(collectible) - 4) < 1e-9 && std::abs(reward(revisit) + 1) < 1e-9, "Circle bonus was not one-time");
    Transition lap{}, continued{};
    lap.after.elapsed = 1; lap.after.lapsCompleted = 1;
    continued.before = lap.after; continued.after = continued.before; continued.after.elapsed = 2;
    check(std::abs(reward(lap) - 19) < 1e-9 && std::abs(reward(continued) + 1) < 1e-9, "Lap bonus was not one-time");
    cool_car::control::RobotNavigation localization(navigationMap(), lidarAngles());
    const Pose2D knownPose{-7, -4, 0};
    const auto scan = localization.expectedRanges(knownPose);
    for (const auto mode : {LocalizationMode::ExtendedKalman, LocalizationMode::UnscentedKalman, LocalizationMode::ParticleFilter}) {
        localization.reset({-6.9, -4.1, 0.02});
        localization.setLocalizationMode(mode);
        for (int observation = 0; observation < 20; ++observation) localization.observe(scan, {0, 1});
        const auto estimate = localization.pose();
        check(std::hypot(estimate.x - knownPose.x, estimate.z - knownPose.z) < 0.2, "Lidar localization did not correct initial pose error");
        const auto blocked = localization.plan({-7, 7, 0}, std::vector<double>(11, 0.3), 0, 0, 2.4);
        check(blocked.speed == 0 && blocked.blocked, "Navigation did not stop for a blocked scan");
    }
    Simulation simulation;
    const auto start = simulation.position();
    for (int stepIndex = 0; stepIndex < 240; ++stepIndex) simulation.step(1.5f, 0);
    check(simulation.position().z - start.z > 2, "Forward drive did not advance the robot");
    check(std::abs(simulation.heading()) < 0.05f, "Straight drive changed the heading");
    for (int stepIndex = 0; stepIndex < 180; ++stepIndex) simulation.step(0, manualTurnCommand(true, false));
    check(simulation.heading() > 2, "Left steering command turned the robot right");
    simulation.setPose(start, 0);
    for (int stepIndex = 0; stepIndex < 180; ++stepIndex) simulation.step(0, manualTurnCommand(false, true));
    check(simulation.heading() < -2, "Right steering command turned the robot left");
    simulation.setPose({-10, 0.42f, 10.5f}, 0);
    check(simulation.beams[5].hit && simulation.beams[5].distance < 1.5f, "Lidar failed to detect the wall");
    for (int stepIndex = 0; stepIndex < 360; ++stepIndex) simulation.step(3, 0);
    check(simulation.position().z < 11.4f, "Robot crossed the collision boundary");
    check(simulation.collisionCount > 0, "Collision telemetry did not register contact");
    check(simulation.localizationError() < 0.3, "Localization integrated commanded motion while the wall blocked the robot");
    simulation.reset();
    check(Vector3Distance(start, simulation.position()) < 0.001f && simulation.elapsed == 0, "Reset did not restore the initial state");
    for (int stepIndex = 0; stepIndex < 9000; ++stepIndex) {
        const auto command = simulation.autopilot(2.4f);
        simulation.step(command.first, command.second);
    }
    check(simulation.laps >= 1, "Autonomous patrol did not complete a lap");
    check(simulation.collisionCount == 0, "Autonomous patrol collided");
    for (const auto& beam : simulation.beams) check(std::isfinite(beam.distance) && beam.distance >= 0 && beam.distance <= sensorRange, "Invalid lidar range");
    check(simulation.localizationError() < 0.3, "EKF localization drifted during patrol");
    RobotEnvironment environment(LocalizationMode::ParticleFilter);
    const auto episode = collectEpisode(environment, 1000);
    check(!episode.empty() && environment.simulation.collisionCount == 0, "Controller episode collided");
    check(environment.simulation.exitReached(), "Particle-localized episode did not reach the exit");
    check(environment.simulation.localizationError() < 0.4, "Particle localization drifted during the episode");
    double totalReward = 0;
    for (const auto& transition : episode) {
        check(transition.reward.has_value(), "Exit reward was not assigned");
        totalReward += *transition.reward;
    }
    check(environment.simulation.circlesCollected > 0, "Driving across a circle did not collect it");
    check(std::abs(totalReward - (100 + circlePoints * environment.simulation.circlesCollected
        + lapPoints * environment.simulation.laps - environment.simulation.elapsed)) < 0.001,
        "Exit return does not match time, circle, and lap points");
    environment.simulation.reset();
    check(environment.simulation.circlesCollected == 0, "Reset did not clear collected circles");
    std::cout << "Robot diagnostics passed: motion, lidar, collisions, EKF/UKF/PF, exit, time cost, one-time circle and lap rewards.\n";
    return 0;
}

void label(Font font, const char* text, float x, float y, float size, Color color = ink) { DrawTextEx(font, text, {x, y}, size, 0, color); }
void captureWindow(const std::string& path) {
    Image image = LoadImageFromScreen();
    const bool exported = ExportImage(image, path.c_str());
    UnloadImage(image);
    if (!exported) throw std::runtime_error("Unable to save screenshot: " + path);
}
bool button(Font font, Rectangle bounds, const char* text, bool selected = false) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRec(bounds, selected ? ink : hover ? Color{226, 234, 229, 255} : Color{238, 242, 238, 255});
    float fontSize = 16;
    auto textSize = MeasureTextEx(font, text, fontSize, 0);
    if (textSize.x > bounds.width - 12) {
        fontSize *= (bounds.width - 12) / textSize.x;
        textSize = MeasureTextEx(font, text, fontSize, 0);
    }
    label(font, text, bounds.x + (bounds.width - textSize.x) / 2, bounds.y + (bounds.height - textSize.y) / 2, fontSize, selected ? WHITE : ink);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
bool driveButton(Rectangle bounds, int direction) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    const bool held = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    DrawRectangleRec(bounds, held ? accent : hover ? Color{223, 232, 227, 255} : Color{234, 240, 235, 255});
    const Vector2 center{bounds.x + bounds.width / 2, bounds.y + bounds.height / 2};
    const Vector2 tip{center.x + std::sin(direction * pi / 2) * 9, center.y - std::cos(direction * pi / 2) * 9};
    const Vector2 base{center.x - std::sin(direction * pi / 2) * 6, center.y + std::cos(direction * pi / 2) * 6};
    const Vector2 side{std::cos(direction * pi / 2) * 7, std::sin(direction * pi / 2) * 7};
    DrawTriangle(tip, Vector2Subtract(base, side), Vector2Add(base, side), held ? WHITE : ink);
    return held;
}

void drawFigTree(Model foliage, float elapsed) {
    const Color bark{114, 113, 93, 255};
    const Vector3 fork{figTreePosition.x - 0.15f, 2.8f, figTreePosition.z};
    DrawCylinderEx(figTreePosition, fork, 0.56f, 0.32f, 14, bark);
    for (int branch = 0; branch < 9; ++branch) {
        const float angle = branch * 2 * pi / 9;
        const Vector3 end{figTreePosition.x + std::cos(angle) * 3.1f, 4.3f + 0.35f * std::sin(angle * 2), figTreePosition.z + std::sin(angle) * 3.1f};
        DrawCylinderEx(fork, end, 0.23f, 0.07f, 10, bark);
        const float sway = std::sin(elapsed * 0.6f + angle) * 0.045f;
        DrawModelEx(foliage, {end.x + sway, end.y + 0.85f, end.z}, {0, 1, 0}, angle * 180 / pi,
            {2.35f, 1.45f, 2.05f}, branch % 2 ? Color{65, 126, 70, 255} : Color{82, 145, 74, 255});
        DrawCylinderEx(figTreePosition, {figTreePosition.x + std::cos(angle) * 1.0f, 0.025f, figTreePosition.z + std::sin(angle) * 1.0f}, 0.15f, 0.025f, 8, bark);
    }
    DrawModelEx(foliage, {figTreePosition.x, 5.6f, figTreePosition.z}, {0, 1, 0}, 0, {3.1f, 1.6f, 3.0f}, {76, 137, 71, 255});
    const std::array<Vector2, 11> leafEdge{{{-0.12f, 0}, {-0.50f, 0.15f}, {-0.22f, 0.42f}, {-0.54f, 0.68f}, {-0.13f, 0.61f},
        {0, 1}, {0.13f, 0.61f}, {0.54f, 0.68f}, {0.22f, 0.42f}, {0.50f, 0.15f}, {0.12f, 0}}};
    for (int leaf = 0; leaf < 160; ++leaf) {
        const float angle = leaf * 2.399963f;
        const float radius = 4.4f + 0.45f * std::sin(leaf * 1.7f);
        const Vector3 center{figTreePosition.x + std::cos(angle) * radius, 4.2f + 1.7f * (0.5f + 0.5f * std::sin(leaf * 0.83f)),
            figTreePosition.z + std::sin(angle) * radius};
        const auto point = [&](Vector2 local) {
            return Vector3{center.x + (local.x * std::cos(angle) - local.y * std::sin(angle)) * 0.55f,
                center.y - local.y * 0.32f, center.z + (local.x * std::sin(angle) + local.y * std::cos(angle)) * 0.55f};
        };
        const auto middle = point({0, 0.45f});
        for (std::size_t edge = 0; edge < leafEdge.size(); ++edge) {
            const auto first = point(leafEdge[edge]), second = point(leafEdge[(edge + 1) % leafEdge.size()]);
            DrawTriangle3D(middle, first, second, {103, 160, 74, 255});
            DrawTriangle3D(middle, second, first, {70, 132, 68, 255});
        }
        DrawLine3D(point({0, 0}), point({0, 0.9f}), {158, 184, 100, 255});
        if (leaf % 5 == 0) DrawSphere({center.x, center.y - 0.25f, center.z}, 0.085f, {105, 73, 101, 255});
    }
}

void drawWorld(Simulation& simulation, Model chassis, Model foliage, bool showSensors) {
    DrawPlane({0, -0.001f, 0}, {24, 24}, {217, 230, 220, 255});
    DrawPlane({-5, -0.002f, 15}, {14, 8}, {202, 222, 192, 255});
    DrawCylinder({figTreePosition.x, 0.005f, figTreePosition.z}, 5.1f, 5.1f, 0.01f, 48, {180, 204, 168, 255});
    DrawCube({exitTarget.x, 0.015f, exitTarget.z}, 2.0f, 0.03f, 1.1f, {211, 183, 94, 255});
    DrawCubeWires({exitTarget.x, 0.02f, exitTarget.z}, 2.0f, 0.04f, 1.1f, {147, 124, 56, 255});
    for (int gridIndex = -12; gridIndex <= 12; ++gridIndex) {
        const Color gridColor = gridIndex % 4 == 0 ? Color{172, 194, 181, 255} : Color{198, 215, 202, 255};
        DrawLine3D({static_cast<float>(gridIndex), 0.01f, -12}, {static_cast<float>(gridIndex), 0.01f, 12}, gridColor);
        DrawLine3D({-12, 0.01f, static_cast<float>(gridIndex)}, {12, 0.01f, static_cast<float>(gridIndex)}, gridColor);
    }
    DrawCube({-12, 0.5f, 0}, 0.4f, 1, 24, {150, 165, 154, 255});
    DrawCube({12, 0.5f, 0}, 0.4f, 1, 24, {150, 165, 154, 255});
    DrawCube({0, 0.5f, -12}, 24, 1, 0.4f, {150, 165, 154, 255});
    for (const auto& wall : exitWalls) DrawCubeV(wall.position, wall.size, {150, 165, 154, 255});
    DrawCube({-8.5f, 1.25f, 12}, 0.25f, 2.5f, 0.25f, accent);
    DrawCube({-5.5f, 1.25f, 12}, 0.25f, 2.5f, 0.25f, accent);
    DrawCube({-7, 2.4f, 12}, 3.2f, 0.3f, 0.25f, accent);
    drawFigTree(foliage, simulation.elapsed);
    for (const auto& obstacle : obstacles) {
        DrawCube({obstacle.position.x + 0.15f, 0.015f, obstacle.position.z + 0.15f}, obstacle.size.x + 0.2f, 0.02f, obstacle.size.z + 0.2f, {168, 188, 174, 255});
        DrawCubeV(obstacle.position, obstacle.size, coral);
        DrawCubeWiresV(obstacle.position, obstacle.size, {155, 74, 62, 255});
    }
    for (std::size_t waypoint = 0; waypoint < waypoints.size(); ++waypoint) {
        const auto position = waypoints[waypoint];
        DrawCylinder({position.x, 0.018f, position.z}, 0.45f, 0.45f, 0.02f, 32,
            simulation.collectedCircles[waypoint] ? Color{155, 174, 161, 255} : Color{230, 183, 66, 255});
        DrawCylinderWires({position.x, 0.03f, position.z}, 0.55f, 0.55f, 0.02f, 32, simulation.collectedCircles[waypoint] ? muted : accent);
    }
    for (std::size_t trailIndex = 1; trailIndex < simulation.trail.size(); ++trailIndex) {
        auto start = simulation.trail[trailIndex - 1], end = simulation.trail[trailIndex];
        start.y = end.y = 0.04f;
        DrawLine3D(start, end, {23, 141, 123, 180});
    }
    if (showSensors) {
        for (std::size_t beamIndex = 0; beamIndex < simulation.beams.size(); ++beamIndex) {
            const auto& beam = simulation.beams[beamIndex];
            const bool free = beamIndex < simulation.drivePlan.freeDirections.size() && simulation.drivePlan.freeDirections[beamIndex];
            DrawLine3D(beam.start, beam.end, free ? Color{30, 150, 120, 190} : Color{217, 104, 76, 190});
            if (beam.hit) DrawSphere(beam.end, 0.055f, coral);
        }
    }
    const auto estimate = simulation.navigation.pose();
    const Vector3 estimatedPosition{static_cast<float>(estimate.x), 0.06f, static_cast<float>(estimate.z)};
    DrawCylinderWires(estimatedPosition, 0.85f, 0.85f, 0.02f, 24, {210, 158, 36, 255});
    DrawLine3D(estimatedPosition, {estimatedPosition.x + static_cast<float>(std::sin(estimate.yaw)), 0.06f,
        estimatedPosition.z + static_cast<float>(std::cos(estimate.yaw))}, {210, 158, 36, 255});
    if (simulation.localizationMode == LocalizationMode::ParticleFilter) {
        for (const auto& particle : simulation.navigation.particlePoses())
            DrawSphere({static_cast<float>(particle.x), 0.08f, static_cast<float>(particle.z)}, 0.035f, {204, 158, 56, 150});
    }
    Vector3 previous = estimatedPosition;
    for (const auto& pose : simulation.drivePlan.path) {
        const Vector3 next{static_cast<float>(pose.x), 0.07f, static_cast<float>(pose.z)};
        DrawLine3D(previous, next, {20, 109, 186, 255});
        previous = next;
    }
    const auto position = simulation.position();
    DrawCylinder({position.x, 0.022f, position.z}, 0.8f, 0.8f, 0.01f, 32, {160, 183, 166, 255});
    DrawModelEx(chassis, position, {0, 1, 0}, simulation.heading() * 180 / pi, {1, 1, 1}, accent);
    DrawModelEx(chassis, simulation.localPoint({0, 0.2f, -0.06f}), {0, 1, 0}, simulation.heading() * 180 / pi, {0.8f, 0.24f, 0.8f}, {234, 241, 233, 255});
    for (int side : {-1, 1}) {
        for (float axle : {-0.4f, 0.4f}) {
            const auto wheelStart = simulation.localPoint({side * 0.39f, -0.16f, axle});
            const auto wheelEnd = simulation.localPoint({side * 0.59f, -0.16f, axle});
            DrawCylinderEx(wheelStart, wheelEnd, 0.23f, 0.23f, 20, {35, 46, 43, 255});
            const float roll = (side < 0 ? simulation.leftTravel : simulation.rightTravel) / 0.23f;
            const auto hub = simulation.localPoint({side * 0.60f, -0.16f, axle});
            const auto spoke = simulation.localPoint({side * 0.60f, -0.16f + std::cos(roll) * 0.16f, axle + std::sin(roll) * 0.16f});
            DrawSphere(hub, 0.055f, {187, 203, 193, 255});
            DrawLine3D(hub, spoke, WHITE);
        }
        DrawSphere(simulation.localPoint({side * 0.23f, 0.015f, 0.64f}), 0.065f, {241, 190, 73, 255});
    }
    DrawCylinderEx(simulation.localPoint({0, 0.23f, 0.35f}), simulation.localPoint({0, 0.38f, 0.35f}), 0.12f, 0.12f, 20, ink);
    DrawSphere(simulation.localPoint({0, 0.40f, 0.35f}), 0.10f, {238, 189, 71, 255});
}

int runWindow(int argc, char** argv) {
    std::string capturePath;
    std::string modelPath = "build/robot-exit-circle-policy.txt";
    bool initialRl = false;
    bool initialTraining = false;
    LocalizationMode initialLocalizer = LocalizationMode::ExtendedKalman;
    bool initialExitMission = false;
    int frameLimit = 0, initialWidth = 1280, initialHeight = 800;
    for (int argument = 1; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (argument + 1 >= argc) throw std::runtime_error("Expected a value after " + option);
        if (option == "--capture") capturePath = argv[++argument];
        else if (option == "--model") modelPath = argv[++argument];
        else if (option == "--training") {
            const std::string value = argv[++argument];
            if (value != "on" && value != "off") throw std::invalid_argument("Training must be on or off");
            initialTraining = value == "on";
            if (initialTraining) initialRl = true;
        }
        else if (option == "--policy") {
            const std::string value = argv[++argument];
            if (value != "rl" && value != "dwa") throw std::invalid_argument("Policy must be rl or dwa");
            initialRl = value == "rl";
        }
        else if (option == "--frames") frameLimit = std::stoi(argv[++argument]);
        else if (option == "--width") initialWidth = std::stoi(argv[++argument]);
        else if (option == "--height") initialHeight = std::stoi(argv[++argument]);
        else if (option == "--mission") {
            const std::string value = argv[++argument];
            if (value != "exit" && value != "patrol") throw std::invalid_argument("Mission must be exit or patrol");
            initialExitMission = value == "exit";
        }
        else if (option == "--localizer") {
            const std::string value = argv[++argument];
            initialLocalizer = parseLocalizer(value);
        }
        else throw std::runtime_error("Unknown option: " + option);
    }
    if (!capturePath.empty() && frameLimit == 0) frameLimit = 180;
    if (initialWidth < 1000 || initialHeight < 700 || frameLimit < 0) throw std::runtime_error("Minimum window size is 1000 x 700; frame count must be nonnegative");
    using cool_car::control::learning::ControllerLearner;
    auto learner = std::filesystem::exists(modelPath) ? ControllerLearner::load(modelPath) : std::make_unique<ControllerLearner>();
    if (initialRl && !initialTraining && learner->episodes() == 0) throw std::invalid_argument("Train an RL checkpoint before launching learned-policy mode");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(initialWidth, initialHeight, "CoolBox | Robot Lab");
    if (!IsWindowReady()) throw std::runtime_error("Unable to create an OpenGL window");
    SetWindowMinSize(1000, 700);
    SetTargetFPS(60);
    Font font = GetFontDefault();
    bool customFont = false;
#ifdef _WIN32
    if (FileExists("C:/Windows/Fonts/bahnschrift.ttf")) {
        font = LoadFontEx("C:/Windows/Fonts/bahnschrift.ttf", 48, nullptr, 0);
        customFont = IsFontValid(font);
        if (!customFont) font = GetFontDefault();
    }
#endif
    Simulation simulation;
    simulation.setLocalizationMode(initialLocalizer);
    simulation.setExitMission(initialExitMission || initialRl);
    Model chassis = LoadModelFromMesh(GenMeshCube(0.86f, 0.34f, 1.22f));
    Model foliage = LoadModelFromMesh(GenMeshSphere(1, 10, 14));
    RenderTexture2D scene = LoadRenderTexture(initialWidth - 300, initialHeight - 268);
    bool autonomous = !capturePath.empty() || initialExitMission || initialRl, paused = false, sensors = true, followCamera = true;
    bool rlActive = initialRl, trainingEpisode = false;
    bool keepTraining = initialTraining, restartTraining = false;
    std::size_t selectedProfile = learner->bestProfile();
    float maximumSpeed = 2.4f, orbitYaw = 0.8f, orbitPitch = 0.65f, cameraDistance = 11;
    float accumulator = 0;
    int frame = 0;
    const auto startRlEpisode = [&](bool training) {
        selectedProfile = training ? learner->selectProfile() : learner->bestProfile();
        simulation.reset();
        simulation.setExitMission(true);
        simulation.setCircleRoute(ControllerLearner::circleRoute(selectedProfile));
        autonomous = true;
        rlActive = true;
        trainingEpisode = training;
        paused = false;
        accumulator = 0;
    };
    if (initialTraining) startRlEpisode(true);
    else if (initialRl) startRlEpisode(false);
    while (!WindowShouldClose()) {
        if (restartTraining && keepTraining && !paused) { startRlEpisode(true); restartTraining = false; }
        const int width = GetScreenWidth(), height = GetScreenHeight();
        const int viewWidth = width - 300, viewHeight = height - 268;
        const float sidebar = static_cast<float>(viewWidth);
        if (scene.texture.width != viewWidth || scene.texture.height != viewHeight) {
            UnloadRenderTexture(scene);
            scene = LoadRenderTexture(viewWidth, viewHeight);
        }
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_TAB)) {
            autonomous = !autonomous;
            trainingEpisode = false;
            keepTraining = restartTraining = false;
            if (!autonomous) rlActive = false;
        }
        if (IsKeyPressed(KEY_C)) followCamera = !followCamera;
        if (IsKeyPressed(KEY_R)) {
            keepTraining = restartTraining = false;
            if (rlActive) startRlEpisode(false);
            else { simulation.reset(); accumulator = 0; }
        }
        const bool mouseOverWorld = CheckCollisionPointRec(GetMousePosition(), {0, 72, static_cast<float>(viewWidth), static_cast<float>(viewHeight)});
        if (mouseOverWorld) {
            cameraDistance = std::clamp(cameraDistance - GetMouseWheelMove(), 5.0f, 22.0f);
            if (!followCamera && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const auto delta = GetMouseDelta();
                orbitYaw -= delta.x * 0.008f;
                orbitPitch = std::clamp(orbitPitch + delta.y * 0.006f, 0.22f, 1.3f);
            }
        }
        BeginDrawing();
        bool captureRequested = false;
        ClearBackground(paper);
        DrawRectangle(0, 0, width, 72, paper);
        DrawRectangle(0, 71, width, 1, lineColor);
        label(font, "ROBOT LAB", 24, 17, 30);
        label(font, "RB-01 / FIELD SIMULATION", 226, 27, 14, muted);
        if (button(font, {static_cast<float>(viewWidth - 248), 18, 52, 36}, paused ? ">" : "||", paused)) paused = !paused;
        if (button(font, {static_cast<float>(viewWidth - 186), 18, 76, 36}, "RESET")) {
            keepTraining = restartTraining = false;
            if (rlActive) startRlEpisode(false);
            else { simulation.reset(); accumulator = 0; }
        }
        if (button(font, {static_cast<float>(viewWidth - 100), 18, 82, 36}, "CAPTURE")) captureRequested = true;
        DrawCircle(width - 266, 36, 5, paused ? Color{213, 163, 62, 255} : accent);
        label(font, paused ? "PAUSED" : "LIVE / 120 Hz", sidebar + 48, 28, 17);
        DrawRectangle(viewWidth, 72, 300, height - 118, paper);
        DrawRectangle(viewWidth, 72, 1, height - 118, lineColor);
        label(font, "DRIVE MODE", sidebar + 24, 96, 15, muted);
        if (button(font, {sidebar + 24, 124, 58, 36}, "MANUAL", !autonomous)) { autonomous = false; rlActive = false; trainingEpisode = false; keepTraining = restartTraining = false; }
        if (button(font, {sidebar + 88, 124, 58, 36}, "PATROL", autonomous && !simulation.exitMission)) {
            simulation.reset(); simulation.setExitMission(false); autonomous = true; paused = false; accumulator = 0; rlActive = trainingEpisode = false; keepTraining = restartTraining = false;
        }
        if (button(font, {sidebar + 152, 124, 58, 36}, "EXIT", autonomous && simulation.exitMission && !rlActive)) {
            simulation.reset(); simulation.setExitMission(true); simulation.setCircleRoute(false); autonomous = true; paused = false; accumulator = 0; rlActive = trainingEpisode = false; keepTraining = restartTraining = false;
        }
        if (button(font, {sidebar + 216, 124, 58, 36}, "RL", autonomous && rlActive)) { keepTraining = restartTraining = false; startRlEpisode(false); }
        if (rlActive) {
            label(font, "RL PROFILE", sidebar + 24, 184, 15, muted);
            label(font, TextFormat("%.1f m/s", ControllerLearner::speedLimit(selectedProfile)), sidebar + 208, 184, 16);
            if (button(font, {sidebar + 24, 208, 250, 28}, keepTraining ? "STOP TRAINING" : "KEEP TRAINING", keepTraining)) {
                keepTraining = !keepTraining;
                restartTraining = false;
                if (keepTraining) startRlEpisode(true);
                else { trainingEpisode = false; paused = true; }
            }
            label(font, TextFormat("EPISODES %d  /  %s", static_cast<int>(learner->episodes()), ControllerLearner::circleRoute(selectedProfile) ? "CIRCLE ROUTE" : "DIRECT EXIT"), sidebar + 24, 239, 12, muted);
        } else {
            label(font, "SPEED LIMIT", sidebar + 24, 184, 15, muted);
            label(font, TextFormat("%.1f m/s", maximumSpeed), sidebar + 208, 184, 16);
            Rectangle speedBar{sidebar + 24, 220, 250, 6};
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), {speedBar.x, speedBar.y - 12, speedBar.width, 30}))
                maximumSpeed = 0.5f + 3.5f * std::clamp((GetMouseX() - speedBar.x) / speedBar.width, 0.0f, 1.0f);
            DrawRectangleRec(speedBar, lineColor);
            DrawRectangleRec({speedBar.x, speedBar.y, speedBar.width * (maximumSpeed - 0.5f) / 3.5f, 6}, accent);
            DrawCircle(static_cast<int>(speedBar.x + speedBar.width * (maximumSpeed - 0.5f) / 3.5f), 223, 7, accent);
        }
        label(font, "LIDAR", sidebar + 24, 256, 15, muted);
        Rectangle sensorBox{sidebar + 250, 254, 22, 22};
        if (CheckCollisionPointRec(GetMousePosition(), sensorBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) sensors = !sensors;
        DrawRectangleLinesEx(sensorBox, 1, muted);
        if (sensors) {
            DrawLineEx({sensorBox.x + 4, sensorBox.y + 11}, {sensorBox.x + 9, sensorBox.y + 16}, 2, accent);
            DrawLineEx({sensorBox.x + 9, sensorBox.y + 16}, {sensorBox.x + 18, sensorBox.y + 5}, 2, accent);
        }
        const bool up = driveButton({sidebar + 119, 296, 48, 42}, 0);
        const bool left = driveButton({sidebar + 63, 344, 48, 42}, 3);
        const bool down = driveButton({sidebar + 119, 344, 48, 42}, 2);
        const bool right = driveButton({sidebar + 175, 344, 48, 42}, 1);
        float speedCommand = 0, turnCommand = 0;
        if (!autonomous) {
            const bool forward = up || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
            const bool reverse = down || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
            const bool steerLeft = left || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
            const bool steerRight = right || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
            speedCommand = (static_cast<int>(forward) - static_cast<int>(reverse)) * maximumSpeed;
            turnCommand = manualTurnCommand(steerLeft, steerRight);
        }
        if (!paused) {
            accumulator += std::min(GetFrameTime(), 0.1f);
            while (accumulator >= timeStep) {
                if (autonomous) {
                    const auto command = simulation.autopilot(rlActive ? static_cast<float>(ControllerLearner::speedLimit(selectedProfile)) : maximumSpeed);
                    speedCommand = command.first;
                    turnCommand = command.second;
                }
                simulation.step(speedCommand, turnCommand);
                if (!autonomous) static_cast<void>(simulation.autopilot(maximumSpeed));
                accumulator -= timeStep;
                const double timeBudget = rlActive ? learner->timeBudget() : 120;
                if (simulation.exitMission && (simulation.exitReached() || simulation.collisionCount > 0 || simulation.elapsed >= timeBudget)) {
                    if (trainingEpisode) {
                        const double reward = simulation.exitReached() && simulation.collisionCount == 0
                            ? 100 + circlePoints * simulation.circlesCollected + lapPoints * simulation.laps - simulation.elapsed
                            : -100 - timeBudget;
                        learner->observe(selectedProfile, reward, simulation.elapsed, simulation.circlesCollected,
                            simulation.exitReached(), simulation.laps);
                        saveLearner(*learner, modelPath);
                        trainingEpisode = false;
                    }
                    restartTraining = keepTraining;
                    paused = !keepTraining;
                    accumulator = 0; break;
                }
            }
        }
        label(font, "CAMERA", sidebar + 24, 416, 15, muted);
        if (button(font, {sidebar + 24, 444, 122, 32}, "FOLLOW", followCamera)) followCamera = true;
        if (button(font, {sidebar + 152, 444, 122, 32}, "ORBIT", !followCamera)) followCamera = false;
        DrawLine(viewWidth + 24, 498, width - 24, 498, lineColor);
        label(font, "LOCALIZATION", sidebar + 24, 512, 15, muted);
        if (button(font, {sidebar + 24, 536, 66, 30}, "EKF", simulation.localizationMode == LocalizationMode::ExtendedKalman))
            simulation.setLocalizationMode(LocalizationMode::ExtendedKalman);
        if (button(font, {sidebar + 98, 536, 66, 30}, "UKF", simulation.localizationMode == LocalizationMode::UnscentedKalman))
            simulation.setLocalizationMode(LocalizationMode::UnscentedKalman);
        if (button(font, {sidebar + 172, 536, 102, 30}, "PARTICLES", simulation.localizationMode == LocalizationMode::ParticleFilter))
            simulation.setLocalizationMode(LocalizationMode::ParticleFilter);
        const auto robotPosition = simulation.position();
        const auto estimatedPose = simulation.navigation.pose();
        label(font, "EST. POSE", sidebar + 24, 582, 13, muted);
        label(font, TextFormat("%+.2f / %+.2f m", estimatedPose.x, estimatedPose.z), sidebar + 134, 580, 16);
        label(font, "ERROR / SIGMA", sidebar + 24, 614, 13, muted);
        label(font, TextFormat("%.2f / %.2f m", simulation.localizationError(), simulation.navigation.positionUncertainty()), sidebar + 163, 612, 15, accent);
        if (height >= 770) {
            label(font, "FRONT RANGE", sidebar + 24, 646, 13, muted);
            label(font, TextFormat("%.2f m", simulation.beams[5].distance), sidebar + 212, 644, 16);
            if (simulation.exitMission) {
                label(font, "RETURN", sidebar + 24, 678, 13, muted);
                const double timeBudget = rlActive ? learner->timeBudget() : 120;
                const double score = simulation.exitReached() && simulation.collisionCount == 0
                    ? 100 + circlePoints * simulation.circlesCollected + lapPoints * simulation.laps - simulation.elapsed
                    : simulation.collisionCount > 0 || simulation.elapsed >= timeBudget ? -100 - timeBudget
                    : circlePoints * simulation.circlesCollected + lapPoints * simulation.laps - simulation.elapsed;
                label(font, TextFormat("%+.2f", score), sidebar + 209, 676, 16);
            } else {
                label(font, "SCORE", sidebar + 24, 678, 13, muted);
                label(font, TextFormat("%+d", lapPoints * simulation.laps), sidebar + 209, 676, 16);
            }
        }
        Camera3D camera{};
        camera.target = {robotPosition.x, 0.3f, robotPosition.z};
        const float clearingFrame = simulation.exitMission && followCamera ? std::clamp((robotPosition.z - 7) / 5, 0.0f, 1.0f) : 0;
        camera.target = Vector3Lerp(camera.target, {-4.7f, 2.5f, 14.4f}, clearingFrame);
        const float framedDistance = cameraDistance + clearingFrame * 7;
        const float cameraYaw = followCamera ? simulation.heading() + pi + 0.45f : orbitYaw;
        const float cameraPitch = followCamera ? 0.67f : orbitPitch;
        camera.position = {camera.target.x + std::sin(cameraYaw) * std::cos(cameraPitch) * framedDistance,
            camera.target.y + std::sin(cameraPitch) * framedDistance,
            camera.target.z + std::cos(cameraYaw) * std::cos(cameraPitch) * framedDistance};
        camera.up = {0, 1, 0}; camera.fovy = 48; camera.projection = CAMERA_PERSPECTIVE;
        BeginTextureMode(scene);
        ClearBackground({227, 236, 229, 255});
        BeginMode3D(camera);
        drawWorld(simulation, chassis, foliage, sensors);
        EndMode3D();
        label(font, clearingFrame > 0.7f ? "FIG CLEARING" : "FLOOR 01", 22, 22, 18);
        if (clearingFrame <= 0.7f) label(font, "24 x 24 m", 22, 48, 14, muted);
        EndTextureMode();
        DrawTextureRec(scene.texture, {0, 0, static_cast<float>(viewWidth), -static_cast<float>(viewHeight)}, {0, 72}, WHITE);

        const int historyTop = height - 196;
        DrawRectangle(0, historyTop, viewWidth, 150, paper);
        DrawLine(0, historyTop, viewWidth, historyTop, lineColor);
        label(font, "EPISODE COSTS", 24, historyTop + 12, 15);
        label(font, TextFormat("TOTAL COST %+.2f", learner->totalCost()), static_cast<float>(viewWidth - 244), historyTop + 12, 15, muted);
        const std::array<float, 7> columns{24, viewWidth * 0.13f, viewWidth * 0.28f, viewWidth * 0.43f, viewWidth * 0.56f, viewWidth * 0.70f, viewWidth * 0.84f};
        const std::array<const char*, 7> headings{"RUN", "SPEED", "TIME", "POINTS", "COST", "RETURN", "RESULT"};
        for (std::size_t column = 0; column < columns.size(); ++column) label(font, headings[column], columns[column], historyTop + 38, 12, muted);
        const auto& history = learner->history();
        const std::size_t firstEpisode = history.size() > 4 ? history.size() - 4 : 0;
        for (std::size_t episode = firstEpisode; episode < history.size(); ++episode) {
            const auto& trial = history[episode];
            const float row = static_cast<float>(historyTop + 62 + (episode - firstEpisode) * 20);
            label(font, TextFormat("%d", static_cast<int>(episode + 1)), columns[0], row, 13);
            label(font, TextFormat("%.1f m/s", ControllerLearner::speedLimit(trial.profile)), columns[1], row, 13);
            label(font, TextFormat("%.2f s", trial.seconds), columns[2], row, 13);
            label(font, TextFormat("+%d", trial.circles * circlePoints + trial.laps * lapPoints), columns[3], row, 13, accent);
            label(font, TextFormat("%+.2f", trial.cost()), columns[4], row, 13);
            label(font, TextFormat("%+.2f", trial.reward), columns[5], row, 13);
            label(font, trial.exitReached ? "EXIT" : "FAILED", columns[6], row, 13, trial.exitReached ? accent : coral);
        }
        if (history.empty()) label(font, "NO COMPLETED TRIALS", 24, historyTop + 68, 13, muted);
        DrawRectangle(0, height - 46, width, 46, paper);
        DrawLine(0, height - 46, width, height - 46, lineColor);
        label(font, simulation.touching ? "CONTACT" : "RB-01 / READY", 24, height - 31, 15, simulation.touching ? coral : accent);
        label(font, TextFormat("T + %.1f s", simulation.elapsed), 212, height - 31, 15, muted);
        if (simulation.exitMission) label(font, TextFormat("CIRCLES %d / 4    LAPS %d", simulation.circlesCollected, simulation.laps), 366, height - 31, 15, muted);
        else label(font, TextFormat("WAYPOINT %d / 4    LAPS %d", static_cast<int>(simulation.waypointIndex + 1), simulation.laps), 366, height - 31, 15, muted);
        label(font, simulation.drivePlan.blocked && autonomous ? "PLANNER / BLOCKED" : trainingEpisode ? "RL / EXPLORING"
            : rlActive ? learner->episodes() > 0 ? "RL / LEARNED" : "RL / UNTRAINED" : "DWA / AVAILABLE", width - 264, height - 31, 14, muted);
        EndDrawing();
        if (captureRequested) captureWindow("robot-lab.png");
        ++frame;
        if (frameLimit > 0 && frame >= frameLimit) {
            if (!capturePath.empty()) captureWindow(capturePath);
            break;
        }
    }
    UnloadRenderTexture(scene);
    UnloadModel(chassis);
    UnloadModel(foliage);
    if (customFont) UnloadFont(font);
    CloseWindow();
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--diagnostic") return diagnose();
        if (argc >= 2 && std::string(argv[1]) == "--train-rl") return trainController(argc, argv);
        if (argc >= 2 && std::string(argv[1]) == "--episode") {
            std::size_t steps = 1000;
            LocalizationMode mode = LocalizationMode::ParticleFilter;
            float speedLimit = 2.4f;
            std::string policy = "dwa", modelPath = "build/robot-exit-circle-policy.txt";
            bool circleRoute = false;
            for (int argument = 2; argument < argc; ++argument) {
                const std::string option = argv[argument];
                if (argument + 1 >= argc) throw std::invalid_argument("Episode option needs a value");
                if (option == "--steps") {
                    const std::string value = argv[++argument];
                    if (value.empty() || value.front() == '-') throw std::invalid_argument("Episode steps must be positive");
                    steps = static_cast<std::size_t>(std::stoul(value));
                    if (steps == 0 || steps > 100000) throw std::invalid_argument("Episode steps must be between 1 and 100000");
                }
                else if (option == "--speed-limit") speedLimit = std::stof(argv[++argument]);
                else if (option == "--model") modelPath = argv[++argument];
                else if (option == "--policy") {
                    policy = argv[++argument];
                    if (policy != "rl" && policy != "dwa") throw std::invalid_argument("Policy must be rl or dwa");
                }
                else if (option == "--localizer") {
                    const std::string value = argv[++argument];
                    mode = parseLocalizer(value);
                } else throw std::invalid_argument("Unknown episode option: " + option);
            }
            if (policy == "rl") {
                const auto learner = cool_car::control::learning::ControllerLearner::load(modelPath);
                if (learner->episodes() == 0) throw std::invalid_argument("RL checkpoint has no training episodes");
                if (learner->stepLimit() != steps) throw std::invalid_argument("Evaluation step limit does not match checkpoint");
                speedLimit = static_cast<float>(cool_car::control::learning::ControllerLearner::speedLimit(learner->bestProfile()));
                circleRoute = cool_car::control::learning::ControllerLearner::circleRoute(learner->bestProfile());
            }
            RobotEnvironment environment(mode, speedLimit, circleRoute);
            const auto trajectory = collectEpisode(environment, steps);
            double totalReward = 0;
            for (const auto& transition : trajectory) totalReward += transition.reward.value_or(0);
            std::cout << "Episode policy=" << policy << " speed=" << speedLimit << " transitions=" << trajectory.size() << " exit_reached=" << environment.simulation.exitReached()
                << " contacts=" << environment.simulation.collisionCount << " localization_error=" << environment.simulation.localizationError()
                << " seconds=" << environment.simulation.elapsed << " circles=" << environment.simulation.circlesCollected
                << " laps=" << environment.simulation.laps
                << " cost=" << (environment.simulation.exitReached() ? 100 - totalReward : -totalReward) << " return=" << totalReward << '\n';
            return environment.simulation.collisionCount == 0 && environment.simulation.exitReached() ? 0 : 1;
        }
        return runWindow(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Robot simulator: " << error.what() << '\n';
        return 1;
    }
}