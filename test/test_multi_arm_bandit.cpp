#include <gtest/gtest.h>
#include "ml/multi_arm_bandit/bandit_arm.h"
#include "ml/multi_arm_bandit/epsilon_greedy_agent.h"
#include "ml/multi_arm_bandit/decaying_epsilon_agent.h"
#include "ml/multi_arm_bandit/ucb_agent.h"
#include "ml/multi_arm_bandit/thompson_sampling_agent.h"

// Test fixture for BanditArm
class BanditArmTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_bandit = new BanditArm(0.5); 
    }

    void TearDown() override {
        delete test_bandit;
    }

    BanditArm* test_bandit;
};

// Test case 1: Verify initial state
TEST_F(BanditArmTest, InitialStateIsCorrect) {
    ASSERT_EQ(test_bandit->get_estimated_prob(), 0.0);
    ASSERT_EQ(test_bandit->get_pull_count(), 0);
}

// Test case 2: Verify a single pull and update
TEST_F(BanditArmTest, SinglePullAndUpdate) {
    double reward = test_bandit->pull();
    test_bandit->update(reward);
    ASSERT_EQ(test_bandit->get_pull_count(), 1);
    ASSERT_EQ(test_bandit->get_estimated_prob(), reward);
}

// Test case 3: Verify multiple updates
TEST_F(BanditArmTest, MultipleUpdates) {
    test_bandit->update(1.0); 
    test_bandit->update(0.0); 
    test_bandit->update(1.0); 
    
    ASSERT_EQ(test_bandit->get_pull_count(), 3);
    ASSERT_NEAR(test_bandit->get_estimated_prob(), 2.0 / 3.0, 0.001); 
}

// Test case 1: Pure exploitation (epsilon = 0)
TEST(EpsilonGreedyAgentTest, ExploitationWorksCorrectly) {
    std::vector<double> true_probs = {0.2, 0.9, 0.5};
    EpsilonGreedyAgent agent(true_probs, 0.0, 2);
    
    // Manually perform a single pull on the best arm to establish it as the best choice.
    // This circumvents the initial state where all probabilities are 0.
    agent.get_bandit(1).update(1.0);
    
    // Simulate 9 more pulls
    agent.run_simulation(9);
    
    // With pure exploitation, the agent should now always choose the best arm (index 1)
    ASSERT_EQ(agent.get_bandit(0).get_pull_count(), 0);
    ASSERT_EQ(agent.get_bandit(1).get_pull_count(), 10);
    ASSERT_EQ(agent.get_bandit(2).get_pull_count(), 0);
}
// Test case 2: Pure exploration (epsilon = 1)
TEST(EpsilonGreedyAgentTest, ExplorationWorksCorrectly) {
    std::vector<double> true_probs = {0.2, 0.8};
    EpsilonGreedyAgent agent(true_probs, 1.0, 2);
    agent.run_simulation(100);
    
    // The number of pulls should be roughly equal for both arms
    ASSERT_NEAR(agent.get_bandit(0).get_pull_count(), 50, 15);
    ASSERT_NEAR(agent.get_bandit(1).get_pull_count(), 50, 15);
}

// Test case 1: Ensure initial pulls are one for each arm
TEST(UCBAgentTest, AllArmsPulledOnceInitially) {
    std::vector<double> true_probs = {0.1, 0.2, 0.3};
    UCBAgent agent(true_probs, 2.0);
    
    // Each arm should be pulled once in the first 3 steps
    agent.run_simulation(3);
    
    ASSERT_EQ(agent.get_bandit(0).get_pull_count(), 1);
    ASSERT_EQ(agent.get_bandit(1).get_pull_count(), 1);
    ASSERT_EQ(agent.get_bandit(2).get_pull_count(), 1);
}

TEST(DecayingEpsilonGreedyAgentTest, RapidDecayLeadsToExploitation) {
    // Agent with a high initial epsilon and a rapid decay rate
    std::vector<double> true_probs = {0.2, 0.9, 0.5};
    double initial_epsilon = 1.0;
    double decay_rate = 1.0; // High decay rate
    
    // Using a fixed seed for deterministic behavior
    DecayingEpsilonGreedyAgent agent(true_probs, initial_epsilon, decay_rate, 123);
    
    // Manually pull the best arm once to give it a head start in estimated reward
    agent.get_bandit(1).update(1.0);
    
    // Run for 50 steps. The decay should make epsilon very small.
    agent.run_simulation(50);
    
    // The agent should have overwhelmingly exploited the best arm (index 1)
    ASSERT_GT(agent.get_bandit(1).get_pull_count(), 40);
    ASSERT_LT(agent.get_bandit(0).get_pull_count(), 5);
    ASSERT_LT(agent.get_bandit(2).get_pull_count(), 5);
}

TEST(DecayingEpsilonGreedyAgentTest, SlowDecayAllowsInitialExploration) {
    // Agent with a moderate initial epsilon and a very slow decay rate
    std::vector<double> true_probs = {0.2, 0.8, 0.5};
    double initial_epsilon = 0.5;
    double decay_rate = 0.01; // Low decay rate
    
    DecayingEpsilonGreedyAgent agent(true_probs, initial_epsilon, decay_rate, 456);
    
    // Run a short simulation. The pulls should be distributed somewhat evenly
    // due to the slow decay.
    agent.run_simulation(30);
    
    // The total number of pulls for all arms should be 30.
    int total_pulls = agent.get_bandit(0).get_pull_count() + 
                      agent.get_bandit(1).get_pull_count() +
                      agent.get_bandit(2).get_pull_count();
    
    ASSERT_EQ(total_pulls, 30);
    
    // Check that each arm has been pulled a reasonable number of times.
    // The pulls won't be perfectly even due to random chance, so we use `ASSERT_GE`.
    ASSERT_GE(agent.get_bandit(0).get_pull_count(), 1);
    ASSERT_GE(agent.get_bandit(1).get_pull_count(), 1);
    ASSERT_GE(agent.get_bandit(2).get_pull_count(), 1);
}

TEST(ThompsonSamplingAgentTest, AllArmsArePulledInitially) {
    std::vector<double> true_probs = {0.2, 0.8, 0.5};
    long long seed = 123;
    
    ThompsonSamplingAgent agent(true_probs, seed);
    
    // Run a short simulation. Each arm should be pulled at least once
    // because the initial Beta distribution (Beta(1,1)) is wide,
    // encouraging exploration.
    agent.run_simulation(10);
    
    // Check that each arm has been pulled at least once.
    ASSERT_GE(agent.get_bandit(0).get_pull_count(), 1);
    ASSERT_GE(agent.get_bandit(1).get_pull_count(), 1);
    ASSERT_GE(agent.get_bandit(2).get_pull_count(), 1);
}
TEST(ThompsonSamplingAgentTest, ExploitationFavorsBestArm) {
    std::vector<double> true_probs = {0.2, 0.8, 0.5};
    long long seed = 456;
    
    ThompsonSamplingAgent agent(true_probs, seed);
    
    // Run for a large number of steps. The agent should eventually converge
    // on the best arm (index 1).
    agent.run_simulation(1000);
    
    // The best arm should have been pulled significantly more than the others.
    // The exact numbers will vary, but we can check the general trend.
    ASSERT_GT(agent.get_bandit(1).get_pull_count(), agent.get_bandit(0).get_pull_count());
    ASSERT_GT(agent.get_bandit(1).get_pull_count(), agent.get_bandit(2).get_pull_count());
}