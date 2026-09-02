
#include "tyst_framework.hpp"
#include "../headers/hidden_markov_model.h"
#include "hmm_flare_adapter.h"
#include "viterbi_rank_convergence.h"

#include <cmath>
#include <limits>

using namespace std;

TEST(HiddenMarkovModel, DeterministicEmissionLogLikelihoodAndViterbi) {
	// Construct a 2-state, 2-observation HMM where state0 always emits 0 and state1 always emits 1.
	HMM model(2, 2);

	std::vector<double> pi = {1.0, 0.0}; // always start in state 0
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
	std::vector<double> pi = {0.2, 0.3, 0.5};
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

TEST(HiddenMarkovModel, BuildsFlareLayersForHmmParameterInference) {
	HmmFlareProblem problem;
	problem.states = 2;
	problem.observations = 2;
	problem.observation_sequences = {{0, 0, 1, 1, 1, 0}, {0, 1, 1, 0}};
	problem.prior.initial_concentration = 1.1;
	problem.prior.transition_concentration = 1.1;
	problem.prior.emission_concentration = 1.1;

	const std::size_t parameter_count = hmm_flare_parameter_count(problem.states, problem.observations);
	EXPECT_EQ(parameter_count, 10u);

	ml::FlareState parameters(parameter_count, 0.0);
	HMM model = hmm_from_flare_state(parameters, problem.states, problem.observations);
	EXPECT_NEAR(model.log_likelihood({0, 1}), std::log(0.25), 1e-12);

	std::vector<ml::FlareLayer> layers = make_hmm_flare_layers(problem, {1, 2});
	ASSERT_EQ(layers.size(), 2u);
	EXPECT_EQ(layers[0].name, std::string("hmm_stride_1"));
	EXPECT_EQ(layers[1].name, std::string("hmm_stride_2"));

	const double fine_log_density = layers[0].log_density(parameters);
	const double coarse_log_density = layers[1].log_density(parameters);
	EXPECT_LT(fine_log_density, coarse_log_density);

	ml::FlareConfig config;
	config.seed = 19u;
	config.inner_steps = 2;
	config.proposal_stddev = 0.15;
	ml::FlareMcmcSampler sampler(layers, config);
	ml::FlareRunResult result = sampler.sample(parameters, 8);
	EXPECT_EQ(result.samples.size(), 8u);
	EXPECT_EQ(result.layer_stats[0].proposals, 8u);
	EXPECT_GT(result.layer_stats[1].proposals, result.layer_stats[0].proposals);
}

TEST(HiddenMarkovModel, StochasticCaseMatchesRankConvergenceDecoder) {
	HMM model(3, 3);

	std::vector<double> pi = {0.55, 0.35, 0.10};
	model.set_initial_probabilities(pi);

	Eigen::MatrixXd A(3, 3);
	A << 0.70, 0.20, 0.10,
		 0.25, 0.60, 0.15,
		 0.15, 0.35, 0.50;
	model.set_transition_matrix(A);

	Eigen::MatrixXd B(3, 3);
	B << 0.65, 0.25, 0.10,
		 0.15, 0.70, 0.15,
		 0.10, 0.20, 0.70;
	model.set_emission_matrix(B);

	std::vector<int> observations = {0, 1, 2, 1, 0, 2, 2, 1};

	auto hmm_path = model.get_most_likely_states(observations);
	ASSERT_EQ(hmm_path.size(), observations.size());

	using Decoder = trekker::algorithm::dynamic_programming::ViterbiRankConvergence<double>;
	Decoder::Vector initial_log(3, -std::numeric_limits<double>::infinity());
	Decoder::Matrix transition_log(3, Decoder::Vector(3, -std::numeric_limits<double>::infinity()));
	Decoder::Matrix emission_log(3, Decoder::Vector(3, -std::numeric_limits<double>::infinity()));

	for (int s = 0; s < 3; ++s) {
		initial_log[s] = std::log(pi[s]);
		for (int n = 0; n < 3; ++n) {
			transition_log[s][n] = std::log(A(s, n));
			emission_log[s][n] = std::log(B(s, n));
		}
	}

	std::vector<std::size_t> obs_indices;
	obs_indices.reserve(observations.size());
	for (int o : observations) obs_indices.push_back(static_cast<std::size_t>(o));

	Decoder::Vector nz(3, 0.0);
	const auto decoded = Decoder::decode_rank_convergence(
		obs_indices,
		initial_log,
		transition_log,
		emission_log,
		3,
		nz);

	ASSERT_EQ(decoded.path.size(), observations.size());
	for (std::size_t i = 0; i < observations.size(); ++i) {
		EXPECT_EQ(hmm_path[i], static_cast<int>(decoded.path[i]));
	}
}

