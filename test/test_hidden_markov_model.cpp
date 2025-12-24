#include <gtest/gtest.h>
#include "ml/hidden_markov_model/hidden_markov_model.h" // Your HMM class header

// Helper function to initialize a simple, known HMM
HMM create_test_hmm() {
    int states = 2; // e.g., "Hot" and "Cold"
    int observations = 3; // e.g., "1", "2", "3" ice creams eaten
    HMM hmm(states, observations);

    // Initial probabilities (pi)
    Eigen::VectorXd pi(states);
    pi << 0.6, 0.4;
    hmm.set_initial_probabilities(pi);

    // Transition matrix (A)
    Eigen::MatrixXd A(states, states);
    A << 0.7, 0.3,
         0.4, 0.6;
    hmm.set_transition_matrix(A);

    // Emission matrix (B)
    Eigen::MatrixXd B(states, observations);
    B << 0.1, 0.4, 0.5,
         0.6, 0.3, 0.1;
    hmm.set_emission_matrix(B);

    return hmm;
}

// Helper for comparing floating-point numbers
// Epsilon for comparison
const double EPSILON = 1e-4;
bool are_matrices_equal(const Eigen::MatrixXd& m1, const Eigen::MatrixXd& m2) {
    return m1.isApprox(m2, EPSILON);
}
bool are_vectors_equal(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2) {
    return v1.isApprox(v2, EPSILON);
}

// Test Fixture for HMM tests
class HMMTest : public ::testing::Test {
protected:
    // Declare a pointer to the HMM object
    HMM* simple_hmm_ptr;
    std::vector<int> observation_sequence;

    void SetUp() override {
        // Allocate the object using the correct constructor
        simple_hmm_ptr = new HMM(2, 3); 
        
        // Initialize the object using the helper function
        *simple_hmm_ptr = create_test_hmm();
        
        // A simple observation sequence
        observation_sequence = {0, 1, 2};
    }

    void TearDown() override {
        // Clean up the allocated memory
        delete simple_hmm_ptr;
    }
};

// --- Test Cases ---
// Test the core Forward Algorithm
TEST_F(HMMTest, ForwardAlgorithmCalculatesCorrectLikelihood) {
    // Known log-likelihood for the simple_hmm and observation_sequence
    // This value would be pre-calculated manually or from a trusted source
    double expected_log_likelihood = -3.3928721329161653;
    double actual_log_likelihood = simple_hmm_ptr->log_likelihood(observation_sequence);
    EXPECT_NEAR(expected_log_likelihood, actual_log_likelihood, EPSILON);
}

// Test the core Viterbi Algorithm
TEST_F(HMMTest, ViterbiAlgorithmFindsMostLikelyStateSequence) {
    // Known most-likely path for the simple_hmm and observation_sequence
    std::vector<int> expected_state_sequence = {1, 0, 0}; // Example: {Cold, Hot, Hot}
    std::vector<int> actual_state_sequence = simple_hmm_ptr->get_most_likely_states(observation_sequence);

    ASSERT_EQ(expected_state_sequence.size(), actual_state_sequence.size());
    for (size_t i = 0; i < expected_state_sequence.size(); ++i) {
        EXPECT_EQ(expected_state_sequence[i], actual_state_sequence[i]);
    }
}

TEST_F(HMMTest, BaumWelchAlgorithmConvergesToCorrectParameters) {
    std::vector<std::vector<int>> training_sequences = {
        {0, 0, 1},
        {1, 2, 2},
        {0, 1, 0},
        {2, 2, 1}
    };
    
    HMM training_hmm(simple_hmm_ptr->get_initial_probabilities().size(), simple_hmm_ptr->get_emission_matrix().cols());
    
    training_hmm.train(training_sequences, 100, 1e-6, 0, 31);

    Eigen::VectorXd expected_pi(2);
    expected_pi << 0.0432047,  0.956795; 
    EXPECT_TRUE(are_vectors_equal(expected_pi, training_hmm.get_initial_probabilities()));

    Eigen::MatrixXd expected_A(2, 2);
    expected_A << 0.0451777,  0.954822,
                  0.556669,  0.443331;
    EXPECT_TRUE(are_matrices_equal(expected_A, training_hmm.get_transition_matrix()));
    
    Eigen::MatrixXd expected_B(2, 3);
    expected_B << 1.82472e-11, 0.659224, 0.340776,
                  0.477907, 0.191988, 0.330105;
    EXPECT_TRUE(are_matrices_equal(expected_B, training_hmm.get_emission_matrix()));
}
