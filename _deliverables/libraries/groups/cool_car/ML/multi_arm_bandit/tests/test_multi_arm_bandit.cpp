#include "tyst_framework.hpp"

#include "bandit_arm.h"
#include "decaying_epsilon_agent.h"
#include "epsilon_greedy_agent.h"
#include "simulation_result.h"
#include "thompson_sampling_agent.h"
#include "ucb_agent.h"

#include <sstream>
#include <vector>
#include <stdexcept>

TYST_TEST(BanditArmTest, UpdateTracksIncrementalAverageAndPullCount) {
	BanditArm arm(0.75);

	TYST_EXPECT_EQ(arm.get_pull_count(), 0);
	TYST_EXPECT_NEAR(arm.get_estimated_prob(), 0.0, 1e-12);
	TYST_EXPECT_NEAR(arm.get_true_prob(), 0.75, 1e-12);

	arm.update(1.0);
	arm.update(0.0);
	arm.update(1.0);

	TYST_EXPECT_EQ(arm.get_pull_count(), 3);
	TYST_EXPECT_NEAR(arm.get_estimated_prob(), 2.0 / 3.0, 1e-12);
}

TYST_TEST(EpsilonGreedyAgentTest, ZeroEpsilonExploitsDeterministicBestArm) {
	EpsilonGreedyAgent agent({1.0, 0.0}, 0.0, 1234);

	agent.run_simulation(5);
	const SimulationResult results = agent.get_results();

	TYST_EXPECT_EQ(results.bandit_results.size(), 2u);
	TYST_EXPECT_EQ(results.bandit_results[0].times_pulled, 5);
	TYST_EXPECT_EQ(results.bandit_results[1].times_pulled, 0);
	TYST_EXPECT_NEAR(results.bandit_results[0].estimated_probability, 1.0, 1e-12);
	TYST_EXPECT_NEAR(results.bandit_results[1].estimated_probability, 0.0, 1e-12);
}

TYST_TEST(DecayingEpsilonGreedyAgentTest, ZeroInitialEpsilonKeepsSelectionDeterministic) {
	DecayingEpsilonGreedyAgent agent({1.0, 0.0}, 0.0, 0.25, 4321);

	agent.run_simulation(4);
	const SimulationResult results = agent.get_results();

	TYST_EXPECT_EQ(results.bandit_results.size(), 2u);
	TYST_EXPECT_EQ(results.bandit_results[0].times_pulled, 4);
	TYST_EXPECT_EQ(results.bandit_results[1].times_pulled, 0);
	TYST_EXPECT_NEAR(results.bandit_results[0].estimated_probability, 1.0, 1e-12);
}

TYST_TEST(ThompsonSamplingAgentTest, SingleArmAlwaysAccumulatesSamples) {
	ThompsonSamplingAgent agent({1.0}, 99);

	agent.run_simulation(4);
	const SimulationResult results = agent.get_results();

	TYST_EXPECT_EQ(results.bandit_results.size(), 1u);
	TYST_EXPECT_EQ(results.bandit_results[0].times_pulled, 4);
	TYST_EXPECT_NEAR(results.bandit_results[0].estimated_probability, 1.0, 1e-12);
}

TYST_TEST(UCBAgentTest, PullsEachArmBeforeApplyingConfidenceBound) {
	UCBAgent agent({1.0, 0.0}, 2.0);

	agent.run_simulation(2);
	const SimulationResult results = agent.get_results();

	TYST_EXPECT_EQ(results.bandit_results.size(), 2u);
	TYST_EXPECT_EQ(results.bandit_results[0].times_pulled, 1);
	TYST_EXPECT_EQ(results.bandit_results[1].times_pulled, 1);
	TYST_EXPECT_NEAR(results.bandit_results[0].estimated_probability, 1.0, 1e-12);
	TYST_EXPECT_NEAR(results.bandit_results[1].estimated_probability, 0.0, 1e-12);
}

TYST_TEST(SimulationResultTest, StreamOutputIncludesHeadersAndArmRows) {
	SimulationResult result;
	result.bandit_results.push_back({1.0, 0.75, 3});
	result.bandit_results.push_back({0.5, 0.25, 1});

	std::ostringstream stream;
	stream << result;
	const std::string text = stream.str();

	TYST_EXPECT_NE(text.find("Simulation Results:"), std::string::npos);
	TYST_EXPECT_NE(text.find("True Prob"), std::string::npos);
	TYST_EXPECT_NE(text.find("0.7500"), std::string::npos);
	TYST_EXPECT_NE(text.find("0.2500"), std::string::npos);
}

TYST_TEST(UCBAgentTest, LearnsFromExternalEpisodeRewards) {
	UCBAgent agent({0, 0}, 0);
	TYST_EXPECT_EQ(agent.select_arm(), 0);
	agent.observe_reward(0, 0.2);
	TYST_EXPECT_EQ(agent.select_arm(), 1);
	agent.observe_reward(1, 0.9);
	TYST_EXPECT_EQ(agent.select_arm(), 1);
	TYST_EXPECT_NEAR(agent.get_results().bandit_results[1].estimated_probability, 0.9, 1e-12);
	TYST_EXPECT_THROW(agent.observe_reward(2, 1), std::invalid_argument);
}
