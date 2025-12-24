#include <gtest/gtest.h>
#include <map>
#include <vector>

#include "ml/bayesian_network/bayesian_network.h"

// Helper function to check if two doubles are approximately equal
bool nearly_equal(double a, double b, double epsilon = 1e-4) {
    return std::abs(a - b) < epsilon;
}

TEST(BayesianNetworkTest, SprinklerNetworkInference) {
    // This test case uses the classic "Sprinkler" network:
    // Cloudy -> Sprinkler
    // Cloudy -> Rain
    // Sprinkler, Rain -> WetGrass

    // 1. Create the network and add nodes
    BayesianNetwork bn;

    int cloudy_idx = bn.add_node("Cloudy", {"false", "true"});
    int sprinkler_idx = bn.add_node("Sprinkler", {"false", "true"});
    int rain_idx = bn.add_node("Rain", {"false", "true"});
    int wet_grass_idx = bn.add_node("WetGrass", {"false", "true"});

    // 2. Add edges
    bn.add_edge(cloudy_idx, sprinkler_idx);
    bn.add_edge(cloudy_idx, rain_idx);
    bn.add_edge(sprinkler_idx, wet_grass_idx);
    bn.add_edge(rain_idx, wet_grass_idx);

    // 3. Set CPTs for each node
    // P(Cloudy)
    Eigen::MatrixXd cloudy_cpt(1, 2);
    cloudy_cpt << 0.5, 0.5;
    bn.set_cpt(cloudy_idx, cloudy_cpt);

    // P(Sprinkler | Cloudy)
    Eigen::MatrixXd sprinkler_cpt(2, 2);
    sprinkler_cpt << 0.5, 0.5, // P(S|C=F)
                     0.9, 0.1; // P(S|C=T)
    bn.set_cpt(sprinkler_idx, sprinkler_cpt);
    
    // P(Rain | Cloudy)
    Eigen::MatrixXd rain_cpt(2, 2);
    rain_cpt << 0.8, 0.2, // P(R|C=F)
                0.2, 0.8; // P(R|C=T)
    bn.set_cpt(rain_idx, rain_cpt);

    // P(WetGrass | Sprinkler, Rain)
    Eigen::MatrixXd wet_grass_cpt(4, 2);
    wet_grass_cpt << 1.0, 0.0, // P(W|S=F, R=F)
                     0.1, 0.9, // P(W|S=F, R=T)
                     0.1, 0.9, // P(W|S=T, R=F)
                     0.01, 0.99; // P(W|S=T, R=T)
    bn.set_cpt(wet_grass_idx, wet_grass_cpt);

    // 4. Test Joint Probability Calculation
    // P(C=T, S=F, R=T, W=T) = P(C=T) * P(S=F|C=T) * P(R=T|C=T) * P(W=T|S=F, R=T)
    //                          = 0.5 * 0.9 * 0.8 * 0.9 = 0.324
    std::map<int, int> assignment;
    assignment[cloudy_idx] = 1; // true
    assignment[sprinkler_idx] = 0; // false
    assignment[rain_idx] = 1; // true
    assignment[wet_grass_idx] = 1; // true
    double joint_prob = bn.calculate_joint_probability(assignment);
    EXPECT_TRUE(nearly_equal(joint_prob, 0.324));

    // 5. Test Inference
    // Query: P(Sprinkler=T | WetGrass=T)
    std::map<int, int> evidence;
    evidence[wet_grass_idx] = 1; // WetGrass=true
    
    // Pre-calculated value for P(Sprinkler=T | WetGrass=T)
    double expected_result = 0.429764; 
    
    double inferred_prob = bn.infer(sprinkler_idx, 1, evidence);
    EXPECT_TRUE(nearly_equal(inferred_prob, expected_result));
}