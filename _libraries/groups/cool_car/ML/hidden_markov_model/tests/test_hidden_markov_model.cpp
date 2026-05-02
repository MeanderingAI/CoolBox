
#include "tyst_framework.hpp"
#include "../headers/hidden_markov_model.h"

using namespace std;

TEST(HiddenMarkovModel, DeterministicEmissionLogLikelihoodAndViterbi) {
	// Construct a 2-state, 2-observation HMM where state0 always emits 0 and state1 always emits 1.
	HMM model(2, 2);

	Eigen::VectorXd pi(2);
	pi << 1.0, 0.0; // always start in state 0
	model.set_initial_probabilities(pi);

	Eigen::MatrixXd A(2,2);
	A << 1.0, 0.0,
		 0.0, 1.0; // deterministic self-transitions
	model.set_transition_matrix(A);

	Eigen::MatrixXd B(2,2);
	B << 1.0, 0.0,
		 0.0, 1.0; // state0->obs0, state1->obs1
	model.set_emission_matrix(B);

	vector<int> obs = {0, 0, 0};

	double ll = model.log_likelihood(obs);
	// Since probabilities are all 1 for the chosen path, log-likelihood should be 0 (log(1)).
	EXPECT_NEAR(ll, 0.0, 1e-12);

	auto path = model.get_most_likely_states(obs);
	ASSERT_EQ(path.size(), obs.size());
	for (auto s : path) EXPECT_EQ(s, 0);
}

TEST(HiddenMarkovModel, GettersAndSetters) {
	HMM model(3, 2);
	Eigen::VectorXd pi(3);
	pi << 0.2, 0.3, 0.5;
	model.set_initial_probabilities(pi);
	EXPECT_EQ(model.get_initial_probabilities().size(), 3);

	Eigen::MatrixXd A = Eigen::MatrixXd::Identity(3,3);
	model.set_transition_matrix(A);
	EXPECT_EQ(model.get_transition_matrix().rows(), 3);

	Eigen::MatrixXd B(3,2);
	B.setZero();
	B(2,1) = 1.0;
	model.set_emission_matrix(B);
	EXPECT_EQ(model.get_emission_matrix().cols(), 2);
}

