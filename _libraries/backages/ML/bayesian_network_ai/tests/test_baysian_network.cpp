#include <gtest/gtest.h>
#include "bayesian_network.h"

#include <cmath>
#include <vector>
#include <map>

static constexpr double TOL = 1e-4;

// ===================================================================
// Structure tests
// ===================================================================

TEST(BayesianNetworkTest, AddNodes) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(bn.num_nodes(), 2);
}

TEST(BayesianNetworkTest, AddNodeDuplicateThrows) {
    BayesianNetwork bn;
    bn.add_node("A", {"T", "F"});
    EXPECT_THROW(bn.add_node("A", {"X"}), std::invalid_argument);
}

TEST(BayesianNetworkTest, AddNodeEmptyStatesThrows) {
    BayesianNetwork bn;
    EXPECT_THROW(bn.add_node("A", {}), std::invalid_argument);
}

TEST(BayesianNetworkTest, AddEdge) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    bn.add_edge(a, b);

    EXPECT_EQ(bn.node(a).children.size(), 1u);
    EXPECT_EQ(bn.node(a).children[0], b);
    EXPECT_EQ(bn.node(b).parents.size(), 1u);
    EXPECT_EQ(bn.node(b).parents[0], a);
}

TEST(BayesianNetworkTest, SelfLoopThrows) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    EXPECT_THROW(bn.add_edge(a, a), std::invalid_argument);
}

TEST(BayesianNetworkTest, CycleThrows) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    int c = bn.add_node("C", {"T", "F"});
    bn.add_edge(a, b);
    bn.add_edge(b, c);
    EXPECT_THROW(bn.add_edge(c, a), std::invalid_argument);
}

TEST(BayesianNetworkTest, AddEdgeByName) {
    BayesianNetwork bn;
    bn.add_node("Rain", {"T", "F"});
    bn.add_node("Wet", {"T", "F"});
    bn.add_edge("Rain", "Wet");
    EXPECT_EQ(bn.node(0).children.size(), 1u);
}

// ===================================================================
// CPT tests
// ===================================================================

TEST(BayesianNetworkTest, SetCPTRoot) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    bn.set_cpt(a, {0.6, 0.4});  // 2 values for 2 states
    // Should not throw
}

TEST(BayesianNetworkTest, SetCPTWithParent) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    bn.add_edge(a, b);
    // 2 parent configs × 2 child states = 4 values
    bn.set_cpt(b, {0.9, 0.1, 0.2, 0.8});
}

TEST(BayesianNetworkTest, SetCPTSizeMismatchThrows) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    EXPECT_THROW(bn.set_cpt(a, {0.6}), std::invalid_argument);
    EXPECT_THROW(bn.set_cpt(a, {0.6, 0.3, 0.1}), std::invalid_argument);
}

// ===================================================================
// Topological order
// ===================================================================

TEST(BayesianNetworkTest, TopologicalOrder) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    int c = bn.add_node("C", {"T", "F"});
    bn.add_edge(a, b);
    bn.add_edge(b, c);

    auto order = bn.topological_order();
    EXPECT_EQ(order.size(), 3u);
    // A must come before B, B before C
    auto pos_a = std::find(order.begin(), order.end(), a) - order.begin();
    auto pos_b = std::find(order.begin(), order.end(), b) - order.begin();
    auto pos_c = std::find(order.begin(), order.end(), c) - order.begin();
    EXPECT_LT(pos_a, pos_b);
    EXPECT_LT(pos_b, pos_c);
}

// ===================================================================
// Inference: simple chain A → B
// ===================================================================

TEST(BayesianNetworkTest, InferencePriorRoot) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    bn.set_cpt(a, {0.6, 0.4});

    auto result = bn.query(a);
    EXPECT_NEAR(result[0], 0.6, TOL);
    EXPECT_NEAR(result[1], 0.4, TOL);
}

TEST(BayesianNetworkTest, InferencePriorChild) {
    // A → B
    // P(A=T)=0.6, P(A=F)=0.4
    // P(B=T|A=T)=0.9, P(B=F|A=T)=0.1
    // P(B=T|A=F)=0.2, P(B=F|A=F)=0.8
    // P(B=T) = 0.6*0.9 + 0.4*0.2 = 0.54 + 0.08 = 0.62

    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    bn.add_edge(a, b);
    bn.set_cpt(a, {0.6, 0.4});
    bn.set_cpt(b, {0.9, 0.1, 0.2, 0.8});

    auto result = bn.query(b);
    EXPECT_NEAR(result[0], 0.62, TOL);
    EXPECT_NEAR(result[1], 0.38, TOL);
}

TEST(BayesianNetworkTest, InferenceWithEvidence) {
    // P(B | A=T) = {0.9, 0.1}
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    bn.add_edge(a, b);
    bn.set_cpt(a, {0.6, 0.4});
    bn.set_cpt(b, {0.9, 0.1, 0.2, 0.8});

    auto result = bn.query(b, {{a, 0}});  // A=T (index 0)
    EXPECT_NEAR(result[0], 0.9, TOL);
    EXPECT_NEAR(result[1], 0.1, TOL);
}

TEST(BayesianNetworkTest, InferenceReverseEvidence) {
    // P(A | B=T) using Bayes' rule:
    // P(A=T|B=T) = P(B=T|A=T)*P(A=T) / P(B=T)
    //            = 0.9*0.6 / 0.62 = 0.54/0.62 ≈ 0.8710
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    int b = bn.add_node("B", {"T", "F"});
    bn.add_edge(a, b);
    bn.set_cpt(a, {0.6, 0.4});
    bn.set_cpt(b, {0.9, 0.1, 0.2, 0.8});

    auto result = bn.query(a, {{b, 0}});  // B=T
    EXPECT_NEAR(result[0], 0.54 / 0.62, TOL);
    EXPECT_NEAR(result[1], 0.08 / 0.62, TOL);
}

// ===================================================================
// Inference: classic Rain-Sprinkler-WetGrass
// ===================================================================

TEST(BayesianNetworkTest, RainSprinklerNetwork) {
    // Rain → Sprinkler, Rain → WetGrass, Sprinkler → WetGrass
    BayesianNetwork bn;
    int rain = bn.add_node("Rain", {"T", "F"});
    int sprinkler = bn.add_node("Sprinkler", {"T", "F"});
    int wet = bn.add_node("WetGrass", {"T", "F"});

    bn.add_edge(rain, sprinkler);
    bn.add_edge(rain, wet);
    bn.add_edge(sprinkler, wet);

    // P(Rain)
    bn.set_cpt(rain, {0.2, 0.8});

    // P(Sprinkler | Rain): R=T → {0.01, 0.99}, R=F → {0.4, 0.6}
    bn.set_cpt(sprinkler, {0.01, 0.99, 0.4, 0.6});

    // P(WetGrass | Sprinkler, Rain)
    // Parents order: [Rain, Sprinkler] → WetGrass
    // R=T,S=T → {0.99, 0.01}
    // R=T,S=F → {0.8, 0.2}
    // R=F,S=T → {0.9, 0.1}
    // R=F,S=F → {0.0, 1.0}
    bn.set_cpt(wet, {
        0.99, 0.01,   // R=T, S=T
        0.80, 0.20,   // R=T, S=F
        0.90, 0.10,   // R=F, S=T
        0.00, 1.00    // R=F, S=F
    });

    // P(Rain | WetGrass=T)
    // By hand: P(W=T) ≈ 0.6471 (can be computed)
    // P(R=T|W=T) should be significantly higher than prior 0.2
    auto result = bn.query(rain, {{wet, 0}});
    EXPECT_GT(result[0], 0.2);  // Observing wet grass increases rain probability
    EXPECT_NEAR(result[0] + result[1], 1.0, TOL);

    // P(Sprinkler | WetGrass=T)
    auto result2 = bn.query(sprinkler, {{wet, 0}});
    EXPECT_GT(result2[0], 0.01 * 0.2 + 0.4 * 0.8);  // Should be higher than prior
    EXPECT_NEAR(result2[0] + result2[1], 1.0, TOL);
}

// ===================================================================
// Inference: three states
// ===================================================================

TEST(BayesianNetworkTest, ThreeStateNode) {
    BayesianNetwork bn;
    int weather = bn.add_node("Weather", {"Sunny", "Cloudy", "Rainy"});
    bn.set_cpt(weather, {0.5, 0.3, 0.2});

    auto result = bn.query(weather);
    EXPECT_NEAR(result[0], 0.5, TOL);
    EXPECT_NEAR(result[1], 0.3, TOL);
    EXPECT_NEAR(result[2], 0.2, TOL);
}

// ===================================================================
// Name-based query
// ===================================================================

TEST(BayesianNetworkTest, QueryByName) {
    BayesianNetwork bn;
    bn.add_node("A", {"T", "F"});
    bn.add_node("B", {"T", "F"});
    bn.add_edge("A", "B");
    bn.set_cpt("A", {0.6, 0.4});
    bn.set_cpt("B", {0.9, 0.1, 0.2, 0.8});

    auto result = bn.query("B", {{"A", "T"}});
    EXPECT_NEAR(result[0], 0.9, TOL);
    EXPECT_NEAR(result[1], 0.1, TOL);
}

// ===================================================================
// Error handling
// ===================================================================

TEST(BayesianNetworkTest, QueryMissingCPTThrows) {
    BayesianNetwork bn;
    int a = bn.add_node("A", {"T", "F"});
    EXPECT_THROW(bn.query(a), std::runtime_error);
}

TEST(BayesianNetworkTest, InvalidNodeIdThrows) {
    BayesianNetwork bn;
    EXPECT_THROW(bn.node(99), std::out_of_range);
}

TEST(BayesianNetworkTest, GetStateIndex) {
    BayesianNetwork bn;
    int a = bn.add_node("Weather", {"Sunny", "Cloudy", "Rainy"});
    EXPECT_EQ(bn.get_state_index(a, "Sunny"), 0);
    EXPECT_EQ(bn.get_state_index(a, "Cloudy"), 1);
    EXPECT_EQ(bn.get_state_index(a, "Rainy"), 2);
    EXPECT_THROW(bn.get_state_index(a, "Snowy"), std::invalid_argument);
}

// ===================================================================
// Factor unit tests
// ===================================================================

TEST(FactorTest, IndexConversion) {
    Factor f({0, 1}, {2, 3}, std::vector<double>(6, 0));
    // 2×3 table: index 0 → (0,0), index 1 → (0,1), ..., index 5 → (1,2)
    auto a = f.index_to_assignment(0);
    EXPECT_EQ(a[0], 0); EXPECT_EQ(a[1], 0);

    a = f.index_to_assignment(4);
    EXPECT_EQ(a[0], 1); EXPECT_EQ(a[1], 1);

    EXPECT_EQ(f.assignment_to_index({1, 2}), 5);
}

TEST(FactorTest, Normalise) {
    Factor f({0}, {2}, {3.0, 7.0});
    f.normalise();
    EXPECT_NEAR(f.table[0], 0.3, TOL);
    EXPECT_NEAR(f.table[1], 0.7, TOL);
}
