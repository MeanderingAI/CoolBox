#include "robot_navigation.h"
#include "robot_learning.h"
#include "controller_learning.h"
#include "tyst_framework.hpp"
#include <cmath>
#include <limits>

using cool_car::control::RobotNavigation;
using cool_car::control::MapBox;
using cool_car::control::Pose2D;

namespace {
std::vector<double> angles() {
    std::vector<double> result;
    for (int beam = -5; beam <= 5; ++beam) result.push_back(beam * 16 * 3.14159265358979323846 / 180);
    return result;
}
std::vector<MapBox> arena() {
    return {{-12.2, -12, -11.8, 12}, {11.8, -12, 12.2, 12}, {-12, -12.2, 12, -11.8}, {-12, 11.8, 12, 12.2}, {-8, -3, -6, -2}};
}
}

TYST_TEST(RobotNavigationTest, PredictsWheelOdometry) {
    RobotNavigation navigation(arena(), angles());
    navigation.reset({-7, -7, 0});
    navigation.predict(0.5, 0);
    TYST_EXPECT_NEAR(navigation.pose().x, -7, 1e-9);
    TYST_EXPECT_NEAR(navigation.pose().z, -6.5, 1e-9);
}
TYST_TEST(RobotNavigationTest, LidarAndForwardVectorCorrectPose) {
    RobotNavigation navigation(arena(), angles());
    const Pose2D truth{-7, -7, 0};
    const auto scan = navigation.expectedRanges(truth);
    navigation.reset({-6.8, -7.15, 0.05});
    for (int observation = 0; observation < 12; ++observation) navigation.observe(scan, {0, 1});
    TYST_EXPECT_NEAR(navigation.pose().x, truth.x, 0.08);
    TYST_EXPECT_NEAR(navigation.pose().z, truth.z, 0.08);
    TYST_EXPECT_NEAR(navigation.pose().yaw, 0, 0.01);
}
TYST_TEST(RobotNavigationTest, HeadingWrapDoesNotJumpToZero) {
    RobotNavigation navigation(arena(), angles());
    navigation.reset({0, 0, 3.13});
    navigation.observe(navigation.expectedRanges(navigation.pose()), {std::sin(-3.13), std::cos(-3.13)});
    TYST_EXPECT_GT(std::abs(navigation.pose().yaw), 3.0);
}
TYST_TEST(RobotNavigationTest, ClearSpaceProducesForwardCommand) {
    RobotNavigation navigation(arena(), angles());
    navigation.reset({0, 0, 0});
    const auto plan = navigation.plan({0, 7, 0}, std::vector<double>(11, 8), 0, 0, 2.4);
    TYST_EXPECT_GT(plan.speed, 0);
    TYST_EXPECT_FALSE(plan.blocked);
    TYST_EXPECT_GT(plan.path.size(), 0u);
}
TYST_TEST(RobotNavigationTest, BlockedOrInvalidScanStopsTheRobot) {
    RobotNavigation navigation(arena(), angles());
    navigation.reset({0, 0, 0});
    const auto blocked = navigation.plan({0, 7, 0}, std::vector<double>(11, 0.3), 0, 0, 2.4);
    TYST_EXPECT_EQ(blocked.speed, 0);
    TYST_EXPECT_TRUE(blocked.blocked);
    auto invalid = std::vector<double>(11, 8);
    invalid[5] = std::numeric_limits<double>::quiet_NaN();
    TYST_EXPECT_EQ(navigation.plan({0, 7, 0}, invalid, 0, 0, 2.4).speed, 0);
}

TYST_TEST(RobotNavigationTest, ConfigurableParticlesLocalizeAgainstLidarMap) {
    RobotNavigation navigation(arena(), angles());
    const Pose2D truth{-7, -7, 0};
    const auto scan = navigation.expectedRanges(truth);
    navigation.reset({-6.9, -7.1, 0.02});
    navigation.setLocalizationMode(cool_car::control::LocalizationMode::ParticleFilter);
    for (int observation = 0; observation < 20; ++observation) navigation.observe(scan, {0, 1});
    TYST_EXPECT_NEAR(navigation.pose().x, truth.x, 0.15);
    TYST_EXPECT_NEAR(navigation.pose().z, truth.z, 0.15);
    TYST_EXPECT_EQ(navigation.particlePoses().size(), 192u);
}

TYST_TEST(RobotNavigationTest, UkfLocalizesAgainstLidarMap) {
    RobotNavigation navigation(arena(), angles());
    const Pose2D truth{-7, -7, 0};
    const auto scan = navigation.expectedRanges(truth);
    navigation.reset({-6.9, -7.1, 0.02});
    navigation.setLocalizationMode(cool_car::control::LocalizationMode::UnscentedKalman);
    for (int observation = 0; observation < 20; ++observation) navigation.observe(scan, {0, 1});
    TYST_EXPECT_NEAR(navigation.pose().x, truth.x, 0.15);
    TYST_EXPECT_NEAR(navigation.pose().z, truth.z, 0.15);
    TYST_EXPECT_TRUE(std::isfinite(navigation.positionUncertainty()));
}

TYST_TEST(RobotLearningTest, CollectsBoundedTransitionsWithoutInventingReward) {
    using namespace cool_car::control::learning;
    class TestEnvironment : public Environment {
    public:
        Observation reset() override { state = {}; return state; }
        EnvironmentStep step(Action action) override { state.speed = action.speed; state.elapsed += 0.1; return {state}; }
        Observation state;
    } environment;
    EpisodeRunner runner;
    const auto trajectory = runner.run(environment, [](const Observation&) { return Action{100, 100}; }, 3);
    TYST_EXPECT_EQ(trajectory.size(), 3u);
    TYST_EXPECT_FALSE(trajectory.front().reward.has_value());
    TYST_EXPECT_NEAR(trajectory.front().action.speed, 4, 1e-12);
    TYST_EXPECT_TRUE(trajectory.back().truncated);
    const auto rewarded = runner.run(environment, [](const Observation&) { return Action{}; }, 1, [](const Transition&) { return 2.0; });
    TYST_EXPECT_NEAR(*rewarded.front().reward, 2, 1e-12);
}

TYST_TEST(RobotLearningTest, FasterExitReceivesHigherReturn) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition fast{}, slow{};
    fast.after.elapsed = 2;
    fast.after.exitReached = fast.terminated = true;
    slow = fast;
    slow.after.elapsed = 5;
    TYST_EXPECT_NEAR(reward(fast), 98, 1e-12);
    TYST_EXPECT_NEAR(reward(slow), 95, 1e-12);
    TYST_EXPECT_GT(reward(fast), reward(slow));
}
TYST_TEST(RobotLearningTest, CollisionCannotEscapeTheRemainingTimeCost) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition collision{}, timeout{};
    collision.after.elapsed = 2;
    collision.after.contact = collision.terminated = true;
    timeout.after.elapsed = 120;
    timeout.truncated = true;
    TYST_EXPECT_NEAR(reward(collision), -220, 1e-12);
    TYST_EXPECT_NEAR(reward(timeout), -220, 1e-12);
}
TYST_TEST(RobotLearningTest, ExitBonusIsOnlyIssuedOnceAndTimeUsesSeconds) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition transition{};
    transition.before.elapsed = 4;
    transition.after.elapsed = 4.5;
    TYST_EXPECT_NEAR(reward(transition), -0.5, 1e-12);
    transition.after.exitReached = true;
    TYST_EXPECT_NEAR(reward(transition), 99.5, 1e-12);
    transition.before.exitReached = true;
    TYST_EXPECT_NEAR(reward(transition), -0.5, 1e-12);
}

TYST_TEST(RobotLearningTest, UcbSupervisorLearnsTheBestObservedController) {
    using namespace cool_car::control::learning;
    ControllerLearner learner(1000, 0);
    for (std::size_t profile = 0; profile < ControllerLearner::profileCount; ++profile) {
        TYST_EXPECT_EQ(learner.selectProfile(), profile);
        learner.observe(profile, 70 + static_cast<double>(profile));
    }
    TYST_EXPECT_EQ(learner.bestProfile(), 11u);
    TYST_EXPECT_EQ(learner.selectProfile(), 11u);
    TYST_EXPECT_EQ(learner.episodes(), 12u);
    TYST_EXPECT_NEAR(learner.stats()[5].meanReturn, 75, 1e-12);
}

TYST_TEST(RobotLearningTest, CircleRewardsOnlyPayForNewCollections) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition transition{};
    transition.after.elapsed = 1;
    transition.after.circlesCollected = 2;
    TYST_EXPECT_NEAR(reward(transition), 9, 1e-12);
    transition.before = transition.after;
    transition.after.elapsed = 2;
    TYST_EXPECT_NEAR(reward(transition), -1, 1e-12);
}
TYST_TEST(RobotLearningTest, CompletedLapsIncreaseReturnOnce) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition transition{};
    transition.after.elapsed = 1;
    transition.after.lapsCompleted = 1;
    TYST_EXPECT_NEAR(reward(transition), 19, 1e-12);
    transition.before = transition.after;
    transition.after.elapsed = 2;
    TYST_EXPECT_NEAR(reward(transition), -1, 1e-12);
}
TYST_TEST(RobotLearningTest, TracksTotalExitCostSeparatelyFromCirclePoints) {
    using namespace cool_car::control::learning;
    ControllerLearner learner;
    learner.observe(3, 95, 10, 1, true);
    learner.observe(4, 131, 9, 4, true, 1);
    TYST_EXPECT_NEAR(learner.totalCost(), -26, 1e-12);
    TYST_EXPECT_EQ(learner.history()[1].circles, 4);
    TYST_EXPECT_EQ(learner.history()[1].laps, 1);
}

TYST_TEST(RobotLearningTest, FailedRunForfeitsCollectedCircleBonus) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition collect{}, fail{};
    collect.after.elapsed = 1;
    collect.after.circlesCollected = 1;
    fail.before = collect.after;
    fail.after = fail.before;
    fail.after.elapsed = 2;
    fail.after.contact = fail.terminated = true;
    TYST_EXPECT_NEAR(reward(collect) + reward(fail), -220, 1e-12);
}
TYST_TEST(RobotLearningTest, FailedRunForfeitsCompletedLapBonus) {
    using namespace cool_car::control::learning;
    const auto reward = exitQuicknessReward();
    Transition lap{}, fail{};
    lap.after.elapsed = 1;
    lap.after.lapsCompleted = 1;
    fail.before = lap.after;
    fail.after = fail.before;
    fail.after.elapsed = 2;
    fail.after.contact = fail.terminated = true;
    TYST_EXPECT_NEAR(reward(lap) + reward(fail), -220, 1e-12);
}