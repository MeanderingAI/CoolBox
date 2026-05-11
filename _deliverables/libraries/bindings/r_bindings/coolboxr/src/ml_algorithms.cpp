#include <Rcpp.h>

#include "decision_tree.h"
#include "support_vector_machine.h"
#include "linear_kernel.h"
#include "rbf_kernel.h"
#include "polynomial_kernel.h"
#include "sigmoid_kernel.h"
#include "bayesian_network.h"
#include "hidden_markov_model.h"
#include "bandit_arm.h"
#include "bandit_agent.h"
#include "epsilon_greedy_agent.h"
#include "ucb_agent.h"
#include "thompson_sampling_agent.h"
#include "decaying_epsilon_agent.h"
#include "marked_point_process.h"
#include "piecewise_conditional_intensity_model.h"
#include "latent_sentiment_analysis.h"
#include "pca.h"
#include "svd.h"
#include "knn.h"
#include "umap.h"

#include "../../../packages/ML/decision_tree/source/decision_tree.cpp"
#include "../../../packages/ML/support_vector_machine/source/linear_kernel.cpp"
#include "../../../packages/ML/support_vector_machine/source/rbf_kernel.cpp"
#include "../../../packages/ML/support_vector_machine/source/polynomial_kernel.cpp"
#include "../../../packages/ML/support_vector_machine/source/sigmoid_kernel.cpp"
#include "../../../packages/ML/support_vector_machine/source/support_vector_machine.cpp"
#include "../../../packages/ML/hidden_markov_model/source/hidden_markov_model.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/bandit_arm.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/bandit_agent.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/epsilon_greedy_agent.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/ucb_agent.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/thompson_sampling_agent.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/decaying_epsilon_agent.cpp"
#include "../../../packages/ML/multi_arm_bandit/source/simulation_result.cpp"
#include "../../../packages/ML/marked_point_process/source/marked_point_process.cpp"
#include "../../../packages/ML/marked_point_process/source/piecewise_conditional_intensity_model.cpp"
#include "../../../packages/ML/latent_sentiment_analysis/source/latent_sentiment_analysis.cpp"
#include "../../../packages/ML/dimensionality_reduction/source/svd.cpp"
#include "../../../packages/ML/dimensionality_reduction/source/pca.cpp"
#include "../../../packages/ML/dimensionality_reduction/source/knn.cpp"
#include "../../../packages/ML/dimensionality_reduction/source/umap.cpp"

namespace {

struct SVMHandle {
    std::unique_ptr<Kernel> kernel;
    std::unique_ptr<SVM> model;
};

mytrix::Matrix to_mytrix_matrix(const Rcpp::NumericMatrix& x) {
    mytrix::Matrix out(x.nrow(), x.ncol());
    for (int i = 0; i < x.nrow(); ++i) {
        for (int j = 0; j < x.ncol(); ++j) {
            out(i, j) = x(i, j);
        }
    }
    return out;
}

mytrix::Vector to_mytrix_vector(const Rcpp::NumericVector& x) {
    mytrix::Vector out(x.size());
    for (int i = 0; i < x.size(); ++i) {
        out(i) = x[i];
    }
    return out;
}

Rcpp::NumericMatrix to_numeric_matrix(const mytrix::Matrix& x) {
    Rcpp::NumericMatrix out(x.rows(), x.cols());
    for (mytrix::Index i = 0; i < x.rows(); ++i) {
        for (mytrix::Index j = 0; j < x.cols(); ++j) {
            out(i, j) = x(i, j);
        }
    }
    return out;
}

Rcpp::IntegerMatrix to_integer_matrix(const mytrix::MatrixI& x, bool one_based = false) {
    Rcpp::IntegerMatrix out(x.rows(), x.cols());
    for (mytrix::Index i = 0; i < x.rows(); ++i) {
        for (mytrix::Index j = 0; j < x.cols(); ++j) {
            out(i, j) = static_cast<int>(x(i, j)) + (one_based ? 1 : 0);
        }
    }
    return out;
}

Rcpp::NumericVector to_numeric_vector(const mytrix::Vector& x) {
    Rcpp::NumericVector out(x.size());
    for (mytrix::Index i = 0; i < x.size(); ++i) {
        out(i) = x(i);
    }
    return out;
}

std::vector<std::vector<int>> to_int_matrix(const Rcpp::IntegerMatrix& x) {
    std::vector<std::vector<int>> out(x.nrow(), std::vector<int>(x.ncol()));
    for (int i = 0; i < x.nrow(); ++i) {
        for (int j = 0; j < x.ncol(); ++j) {
            out[i][j] = x(i, j);
        }
    }
    return out;
}

std::vector<std::vector<double>> to_double_sequences(const Rcpp::List& sequences) {
    std::vector<std::vector<double>> out;
    out.reserve(sequences.size());
    for (int i = 0; i < sequences.size(); ++i) {
        out.push_back(Rcpp::as<std::vector<double>>(sequences[i]));
    }
    return out;
}

std::vector<std::vector<int>> to_int_sequences(const Rcpp::List& sequences) {
    std::vector<std::vector<int>> out;
    out.reserve(sequences.size());
    for (int i = 0; i < sequences.size(); ++i) {
        out.push_back(Rcpp::as<std::vector<int>>(sequences[i]));
    }
    return out;
}

std::vector<mytrix::Matrix> to_matrix_list(const Rcpp::List& matrices) {
    std::vector<mytrix::Matrix> out;
    out.reserve(matrices.size());
    for (int i = 0; i < matrices.size(); ++i) {
        out.push_back(to_eigen_matrix(Rcpp::as<Rcpp::NumericMatrix>(matrices[i])));
    }
    return out;
}

std::map<int, int> to_int_map(const Rcpp::IntegerVector& keys, const Rcpp::IntegerVector& values) {
    if (keys.size() != values.size()) {
        Rcpp::stop("Key and value vectors must have the same length.");
    }

    std::map<int, int> out;
    for (int i = 0; i < keys.size(); ++i) {
        out[keys[i]] = values[i];
    }
    return out;
}

int bn_cpt_row_index(const BayesianNetwork& model, const BayesianNetwork::Node& node, const std::map<int, int>& assignment) {
    int row_index = 0;
    int stride = 1;
    for (int i = static_cast<int>(node.parents.size()) - 1; i >= 0; --i) {
        int parent_id = node.parents[static_cast<size_t>(i)];
        auto it = assignment.find(parent_id);
        if (it == assignment.end()) {
            Rcpp::stop("Assignment is missing a parent state.");
        }
        row_index += it->second * stride;
        stride *= static_cast<int>(model.node(parent_id).states.size());
    }
    return row_index;
}

std::unique_ptr<Kernel> create_kernel(const std::string& kernel_type, double gamma, double coef0, int degree) {
    if (kernel_type == "linear") {
        return std::make_unique<LinearKernel>();
    }
    if (kernel_type == "rbf") {
        return std::make_unique<RBFKernel>(gamma);
    }
    if (kernel_type == "polynomial") {
        return std::make_unique<PolynomialKernel>(gamma, coef0, degree);
    }
    if (kernel_type == "sigmoid") {
        return std::make_unique<SigmoidKernel>(gamma, coef0);
    }

    Rcpp::stop("Unsupported kernel type.");
    return nullptr;
}

PiecewiseConditionalIntensityModel::IntensityType parse_intensity_type(const std::string& type) {
    if (type == "constant") {
        return PiecewiseConditionalIntensityModel::IntensityType::CONSTANT;
    }
    if (type == "linear") {
        return PiecewiseConditionalIntensityModel::IntensityType::LINEAR;
    }
    if (type == "exponential") {
        return PiecewiseConditionalIntensityModel::IntensityType::EXPONENTIAL;
    }
    if (type == "hawkes") {
        return PiecewiseConditionalIntensityModel::IntensityType::HAWKES;
    }
    if (type == "cox") {
        return PiecewiseConditionalIntensityModel::IntensityType::COX;
    }

    Rcpp::stop("Unsupported intensity type.");
    return PiecewiseConditionalIntensityModel::IntensityType::HAWKES;
}

std::string intensity_type_name(PiecewiseConditionalIntensityModel::IntensityType type) {
    switch (type) {
        case PiecewiseConditionalIntensityModel::IntensityType::CONSTANT:
            return "constant";
        case PiecewiseConditionalIntensityModel::IntensityType::LINEAR:
            return "linear";
        case PiecewiseConditionalIntensityModel::IntensityType::EXPONENTIAL:
            return "exponential";
        case PiecewiseConditionalIntensityModel::IntensityType::HAWKES:
            return "hawkes";
        case PiecewiseConditionalIntensityModel::IntensityType::COX:
            return "cox";
        default:
            return "unknown";
    }
}

Rcpp::List simulation_result_to_list(const SimulationResult& result) {
    Rcpp::NumericVector true_probability(result.bandit_results.size());
    Rcpp::NumericVector estimated_probability(result.bandit_results.size());
    Rcpp::IntegerVector times_pulled(result.bandit_results.size());

    for (size_t i = 0; i < result.bandit_results.size(); ++i) {
        true_probability[static_cast<R_xlen_t>(i)] = result.bandit_results[i].true_probability;
        estimated_probability[static_cast<R_xlen_t>(i)] = result.bandit_results[i].estimated_probability;
        times_pulled[static_cast<R_xlen_t>(i)] = result.bandit_results[i].times_pulled;
    }

    return Rcpp::List::create(
        Rcpp::Named("true_probability") = true_probability,
        Rcpp::Named("estimated_probability") = estimated_probability,
        Rcpp::Named("times_pulled") = times_pulled
    );
}

} // namespace

extern "C" SEXP _coolboxr_fit_decision_tree(SEXP xSEXP, SEXP ySEXP, SEXP criterionSEXP, SEXP maxDepthSEXP) {
    BEGIN_RCPP

    Rcpp::IntegerMatrix x(xSEXP);
    Rcpp::IntegerVector y(ySEXP);
    std::string criterion = Rcpp::as<std::string>(criterionSEXP);
    int max_depth = Rcpp::as<int>(maxDepthSEXP);

    auto* tree = new DecisionTree(
        criterion == "entropy" ? SplitCriterion::ENTROPY : SplitCriterion::GINI
    );
    tree->fit(to_int_matrix(x), Rcpp::as<std::vector<int>>(y), max_depth);

    return Rcpp::XPtr<DecisionTree>(tree, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_predict_decision_tree(SEXP ptrSEXP, SEXP newdataSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<DecisionTree> tree(ptrSEXP);
    Rcpp::IntegerMatrix newdata(newdataSEXP);
    Rcpp::IntegerVector predictions(newdata.nrow());

    for (int i = 0; i < newdata.nrow(); ++i) {
        std::vector<int> sample(newdata.ncol());
        for (int j = 0; j < newdata.ncol(); ++j) {
            sample[j] = newdata(i, j);
        }
        predictions[i] = tree->predict(sample);
    }

    return predictions;

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_svm(SEXP xSEXP, SEXP ySEXP, SEXP kernelSEXP, SEXP gammaSEXP, SEXP coef0SEXP, SEXP degreeSEXP) {
    BEGIN_RCPP

    Rcpp::NumericMatrix x(xSEXP);
    Rcpp::NumericVector y(ySEXP);
    std::string kernel_type = Rcpp::as<std::string>(kernelSEXP);
    double gamma = Rcpp::as<double>(gammaSEXP);
    double coef0 = Rcpp::as<double>(coef0SEXP);
    int degree = Rcpp::as<int>(degreeSEXP);

    auto* handle = new SVMHandle();
    handle->kernel = create_kernel(kernel_type, gamma, coef0, degree);
    handle->model = std::make_unique<SVM>(*handle->kernel);
    handle->model->fit(to_eigen_matrix(x), to_eigen_vector(y));

    return Rcpp::XPtr<SVMHandle>(handle, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_predict_svm(SEXP ptrSEXP, SEXP newdataSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<SVMHandle> handle(ptrSEXP);
    Rcpp::NumericMatrix newdata(newdataSEXP);
    Rcpp::NumericVector predictions(newdata.nrow());

    for (int i = 0; i < newdata.nrow(); ++i) {
        mytrix::Vector sample(newdata.ncol());
        for (int j = 0; j < newdata.ncol(); ++j) {
            sample(j) = newdata(i, j);
        }
        predictions[i] = handle->model->predict(sample);
    }

    return predictions;

    END_RCPP
}

extern "C" SEXP _coolboxr_create_bayesian_network() {
    BEGIN_RCPP

    return Rcpp::XPtr<BayesianNetwork>(new BayesianNetwork(), true);

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_add_node(SEXP ptrSEXP, SEXP nameSEXP, SEXP statesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    std::string name = Rcpp::as<std::string>(nameSEXP);
    std::vector<std::string> states = Rcpp::as<std::vector<std::string>>(statesSEXP);

    return Rcpp::wrap(model->add_node(name, states));

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_add_edge(SEXP ptrSEXP, SEXP parentSEXP, SEXP childSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    model->add_edge(Rcpp::as<int>(parentSEXP), Rcpp::as<int>(childSEXP));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_set_cpt(SEXP ptrSEXP, SEXP nodeSEXP, SEXP cptSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    Rcpp::NumericMatrix cpt(cptSEXP);
    model->set_cpt(Rcpp::as<int>(nodeSEXP), Rcpp::as<std::vector<double>>(Rcpp::as<Rcpp::NumericVector>(cpt)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_get_node_id(SEXP ptrSEXP, SEXP nameSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    return Rcpp::wrap(model->get_node_id(Rcpp::as<std::string>(nameSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_query(SEXP ptrSEXP, SEXP queryNodeSEXP, SEXP evidenceNodesSEXP, SEXP evidenceStatesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    auto evidence = to_int_map(Rcpp::IntegerVector(evidenceNodesSEXP), Rcpp::IntegerVector(evidenceStatesSEXP));
    auto result = model->query(Rcpp::as<int>(queryNodeSEXP), evidence);
    return Rcpp::wrap(result);

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_joint_probability(SEXP ptrSEXP, SEXP assignmentSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    Rcpp::IntegerVector assignment(assignmentSEXP);

    if (assignment.size() != model->num_nodes()) {
        Rcpp::stop("Assignment length must match the number of nodes.");
    }

    std::map<int, int> state_assignment;
    for (int i = 0; i < assignment.size(); ++i) {
        state_assignment[i] = assignment[i];
    }

    double probability = 1.0;
    for (int node_id : model->topological_order()) {
        const auto& node = model->node(node_id);
        int state_index = state_assignment.at(node_id);
        int row_index = bn_cpt_row_index(*model, node, state_assignment);
        int flat_index = row_index * static_cast<int>(node.states.size()) + state_index;
        if (flat_index < 0 || flat_index >= static_cast<int>(node.cpt.size())) {
            Rcpp::stop("Assignment produced an invalid CPT index.");
        }
        probability *= node.cpt[static_cast<size_t>(flat_index)];
    }

    return Rcpp::wrap(probability);

    END_RCPP
}

extern "C" SEXP _coolboxr_bn_nodes(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<BayesianNetwork> model(ptrSEXP);
    const auto& nodes = model->nodes();
    Rcpp::List out(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i) {
        out[static_cast<R_xlen_t>(i)] = Rcpp::List::create(
            Rcpp::Named("id") = nodes[i].id + 1,
            Rcpp::Named("name") = nodes[i].name,
            Rcpp::Named("states") = nodes[i].states,
            Rcpp::Named("parents") = Rcpp::wrap(nodes[i].parents),
            Rcpp::Named("children") = Rcpp::wrap(nodes[i].children),
            Rcpp::Named("cpt") = Rcpp::wrap(nodes[i].cpt)
        );
    }

    return out;

    END_RCPP
}

extern "C" SEXP _coolboxr_create_hmm(SEXP statesSEXP, SEXP observationsSEXP) {
    BEGIN_RCPP

    return Rcpp::XPtr<HMM>(new HMM(Rcpp::as<int>(statesSEXP), Rcpp::as<int>(observationsSEXP)), true);

    END_RCPP
}

extern "C" SEXP _coolboxr_hmm_set_parameters(SEXP ptrSEXP, SEXP piSEXP, SEXP transitionSEXP, SEXP emissionSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<HMM> model(ptrSEXP);
    model->set_initial_probabilities(to_eigen_vector(Rcpp::NumericVector(piSEXP)));
    model->set_transition_matrix(to_eigen_matrix(Rcpp::NumericMatrix(transitionSEXP)));
    model->set_emission_matrix(to_eigen_matrix(Rcpp::NumericMatrix(emissionSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_hmm_get_parameters(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<HMM> model(ptrSEXP);
    return Rcpp::List::create(
        Rcpp::Named("initial_probabilities") = to_numeric_vector(model->get_initial_probabilities()),
        Rcpp::Named("transition_matrix") = to_numeric_matrix(model->get_transition_matrix()),
        Rcpp::Named("emission_matrix") = to_numeric_matrix(model->get_emission_matrix())
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_hmm_train(SEXP ptrSEXP, SEXP sequencesSEXP, SEXP maxIterSEXP, SEXP toleranceSEXP, SEXP smoothingSEXP, SEXP seedSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<HMM> model(ptrSEXP);
    model->train(
        to_int_sequences(Rcpp::List(sequencesSEXP)),
        Rcpp::as<int>(maxIterSEXP),
        Rcpp::as<double>(toleranceSEXP),
        Rcpp::as<double>(smoothingSEXP),
        static_cast<unsigned int>(Rcpp::as<int>(seedSEXP))
    );
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_hmm_viterbi(SEXP ptrSEXP, SEXP observationsSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<HMM> model(ptrSEXP);
    return Rcpp::wrap(model->get_most_likely_states(Rcpp::as<std::vector<int>>(observationsSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_hmm_log_likelihood(SEXP ptrSEXP, SEXP observationsSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<HMM> model(ptrSEXP);
    return Rcpp::wrap(model->log_likelihood(Rcpp::as<std::vector<int>>(observationsSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_run_bandit_simulation(SEXP trueProbsSEXP, SEXP strategySEXP, SEXP stepsSEXP, SEXP epsilonSEXP, SEXP cSEXP, SEXP decayRateSEXP, SEXP seedSEXP) {
    BEGIN_RCPP

    std::vector<double> true_probs = Rcpp::as<std::vector<double>>(trueProbsSEXP);
    std::string strategy = Rcpp::as<std::string>(strategySEXP);
    int steps = Rcpp::as<int>(stepsSEXP);
    double epsilon = Rcpp::as<double>(epsilonSEXP);
    double c = Rcpp::as<double>(cSEXP);
    double decay_rate = Rcpp::as<double>(decayRateSEXP);
    long long seed = static_cast<long long>(Rcpp::as<double>(seedSEXP));

    std::unique_ptr<BanditAgent> agent;
    if (strategy == "epsilon_greedy") {
        agent = std::make_unique<EpsilonGreedyAgent>(true_probs, epsilon, seed);
    } else if (strategy == "ucb") {
        agent = std::make_unique<UCBAgent>(true_probs, c);
    } else if (strategy == "thompson_sampling") {
        agent = std::make_unique<ThompsonSamplingAgent>(true_probs, seed);
    } else if (strategy == "decaying_epsilon") {
        agent = std::make_unique<DecayingEpsilonGreedyAgent>(true_probs, epsilon, decay_rate, seed);
    } else {
        Rcpp::stop("Unsupported bandit strategy.");
    }

    agent->run_simulation(steps);
    return simulation_result_to_list(agent->get_results());

    END_RCPP
}

extern "C" SEXP _coolboxr_create_marked_point_process(SEXP numMarksSEXP, SEXP learningRateSEXP, SEXP maxIterationsSEXP) {
    BEGIN_RCPP

    return Rcpp::XPtr<MarkedPointProcess>(
        new MarkedPointProcess(Rcpp::as<int>(numMarksSEXP), Rcpp::as<double>(learningRateSEXP), Rcpp::as<int>(maxIterationsSEXP)),
        true
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_mpp_fit(SEXP ptrSEXP, SEXP eventTimesSEXP, SEXP eventMarksSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<MarkedPointProcess> model(ptrSEXP);
    model->fit(to_double_sequences(Rcpp::List(eventTimesSEXP)), to_int_sequences(Rcpp::List(eventMarksSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_mpp_predict_intensity(SEXP ptrSEXP, SEXP timeSEXP, SEXP historyTimesSEXP, SEXP historyMarksSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<MarkedPointProcess> model(ptrSEXP);
    auto intensities = model->predict_intensity(
        Rcpp::as<double>(timeSEXP),
        Rcpp::as<std::vector<double>>(historyTimesSEXP),
        Rcpp::as<std::vector<int>>(historyMarksSEXP)
    );
    return to_numeric_vector(intensities);

    END_RCPP
}

extern "C" SEXP _coolboxr_mpp_generate_sequence(SEXP ptrSEXP, SEXP timeHorizonSEXP, SEXP maxEventsSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<MarkedPointProcess> model(ptrSEXP);
    auto generated = model->generate_sequence(Rcpp::as<double>(timeHorizonSEXP), Rcpp::as<int>(maxEventsSEXP));
    return Rcpp::List::create(
        Rcpp::Named("event_times") = generated.first,
        Rcpp::Named("event_marks") = generated.second
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_mpp_log_likelihood(SEXP ptrSEXP, SEXP eventTimesSEXP, SEXP eventMarksSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<MarkedPointProcess> model(ptrSEXP);
    return Rcpp::wrap(model->log_likelihood(to_double_sequences(Rcpp::List(eventTimesSEXP)), to_int_sequences(Rcpp::List(eventMarksSEXP))));

    END_RCPP
}

extern "C" SEXP _coolboxr_mpp_parameters(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<MarkedPointProcess> model(ptrSEXP);
    return Rcpp::List::create(
        Rcpp::Named("base_intensity") = to_numeric_vector(model->get_base_intensity()),
        Rcpp::Named("excitation_matrix") = to_numeric_matrix(model->get_excitation_matrix()),
        Rcpp::Named("decay_rate") = model->get_decay_rate()
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_create_pcim(SEXP numIntervalsSEXP, SEXP learningRateSEXP, SEXP maxIterationsSEXP) {
    BEGIN_RCPP

    return Rcpp::XPtr<PiecewiseConditionalIntensityModel>(
        new PiecewiseConditionalIntensityModel(Rcpp::as<int>(numIntervalsSEXP), Rcpp::as<double>(learningRateSEXP), Rcpp::as<int>(maxIterationsSEXP)),
        true
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_create_uniform_intervals(SEXP ptrSEXP, SEXP timeMinSEXP, SEXP timeMaxSEXP, SEXP typeSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    model->create_uniform_intervals(Rcpp::as<double>(timeMinSEXP), Rcpp::as<double>(timeMaxSEXP), parse_intensity_type(Rcpp::as<std::string>(typeSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_create_adaptive_intervals(SEXP ptrSEXP, SEXP eventTimesSEXP, SEXP typeSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    model->create_adaptive_intervals(Rcpp::as<std::vector<double>>(eventTimesSEXP), parse_intensity_type(Rcpp::as<std::string>(typeSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_fit(SEXP ptrSEXP, SEXP eventTimesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    model->fit(to_double_sequences(Rcpp::List(eventTimesSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_fit_with_covariates(SEXP ptrSEXP, SEXP eventTimesSEXP, SEXP covariatesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    model->fit_with_covariates(to_double_sequences(Rcpp::List(eventTimesSEXP)), to_matrix_list(Rcpp::List(covariatesSEXP)));
    return R_NilValue;

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_predict_intensity(SEXP ptrSEXP, SEXP timeSEXP, SEXP historyTimesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    return Rcpp::wrap(model->predict_intensity(Rcpp::as<double>(timeSEXP), Rcpp::as<std::vector<double>>(historyTimesSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_predict_intensity_with_covariates(SEXP ptrSEXP, SEXP timeSEXP, SEXP historyTimesSEXP, SEXP covariatesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    return Rcpp::wrap(model->predict_intensity_with_covariates(
        Rcpp::as<double>(timeSEXP),
        Rcpp::as<std::vector<double>>(historyTimesSEXP),
        to_eigen_vector(Rcpp::NumericVector(covariatesSEXP))
    ));

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_generate_sequence(SEXP ptrSEXP, SEXP timeHorizonSEXP, SEXP maxEventsSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    return Rcpp::wrap(model->generate_sequence(Rcpp::as<double>(timeHorizonSEXP), Rcpp::as<int>(maxEventsSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_log_likelihood(SEXP ptrSEXP, SEXP eventTimesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    return Rcpp::wrap(model->log_likelihood(to_double_sequences(Rcpp::List(eventTimesSEXP))));

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_information_criteria(SEXP ptrSEXP, SEXP eventTimesSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    auto criteria = model->compute_information_criteria(to_double_sequences(Rcpp::List(eventTimesSEXP)));
    return Rcpp::List::create(Rcpp::Named("aic") = criteria.first, Rcpp::Named("bic") = criteria.second);

    END_RCPP
}

extern "C" SEXP _coolboxr_pcim_intervals(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<PiecewiseConditionalIntensityModel> model(ptrSEXP);
    auto intervals = model->get_intervals();
    Rcpp::List out(intervals.size());
    for (size_t i = 0; i < intervals.size(); ++i) {
        out[static_cast<R_xlen_t>(i)] = Rcpp::List::create(
            Rcpp::Named("start_time") = intervals[i].start_time,
            Rcpp::Named("end_time") = intervals[i].end_time,
            Rcpp::Named("intensity_type") = intensity_type_name(intervals[i].intensity_type),
            Rcpp::Named("parameters") = to_numeric_vector(intervals[i].parameters)
        );
    }
    return out;

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_latent_sentiment_analysis(SEXP matrixSEXP, SEXP latentFeaturesSEXP, SEXP learningRateSEXP, SEXP lambdaSEXP, SEXP maxIterationsSEXP) {
    BEGIN_RCPP

    auto* model = new LatentSentimentAnalysis(
        Rcpp::as<int>(latentFeaturesSEXP),
        Rcpp::as<double>(learningRateSEXP),
        Rcpp::as<double>(lambdaSEXP),
        Rcpp::as<int>(maxIterationsSEXP)
    );
    model->train(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    return Rcpp::XPtr<LatentSentimentAnalysis>(model, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_lsa_predict_score(SEXP ptrSEXP, SEXP docIndexSEXP, SEXP termIndexSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<LatentSentimentAnalysis> model(ptrSEXP);
    return Rcpp::wrap(model->predict_score(Rcpp::as<int>(docIndexSEXP), Rcpp::as<int>(termIndexSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_lsa_factors(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<LatentSentimentAnalysis> model(ptrSEXP);
    return Rcpp::List::create(
        Rcpp::Named("document_features") = to_numeric_matrix(model->get_document_features()),
        Rcpp::Named("term_features") = to_numeric_matrix(model->get_term_features())
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_svd(SEXP matrixSEXP, SEXP fullMatricesSEXP) {
    BEGIN_RCPP

    auto* model = new dimensionality_reduction::SVD(Rcpp::as<bool>(fullMatricesSEXP));
    model->compute(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    return Rcpp::XPtr<dimensionality_reduction::SVD>(model, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_svd_summary(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::SVD> model(ptrSEXP);
    return Rcpp::List::create(
        Rcpp::Named("u") = to_numeric_matrix(model->get_U()),
        Rcpp::Named("v") = to_numeric_matrix(model->get_V()),
        Rcpp::Named("s") = to_numeric_vector(model->get_singular_values()),
        Rcpp::Named("explained_variance_ratio") = to_numeric_vector(model->explained_variance_ratio()),
        Rcpp::Named("rank") = model->rank()
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_svd_reconstruct(SEXP ptrSEXP, SEXP numComponentsSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::SVD> model(ptrSEXP);
    return to_numeric_matrix(model->reconstruct(Rcpp::as<int>(numComponentsSEXP)));

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_pca(SEXP matrixSEXP, SEXP componentsSEXP, SEXP centerSEXP, SEXP scaleSEXP) {
    BEGIN_RCPP

    auto* model = new dimensionality_reduction::PCA(
        Rcpp::as<int>(componentsSEXP),
        Rcpp::as<bool>(centerSEXP),
        Rcpp::as<bool>(scaleSEXP)
    );
    model->fit(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    return Rcpp::XPtr<dimensionality_reduction::PCA>(model, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_pca_transform(SEXP ptrSEXP, SEXP matrixSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::PCA> model(ptrSEXP);
    return to_numeric_matrix(model->transform(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP))));

    END_RCPP
}

extern "C" SEXP _coolboxr_pca_inverse_transform(SEXP ptrSEXP, SEXP matrixSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::PCA> model(ptrSEXP);
    return to_numeric_matrix(model->inverse_transform(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP))));

    END_RCPP
}

extern "C" SEXP _coolboxr_pca_summary(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::PCA> model(ptrSEXP);
    return Rcpp::List::create(
        Rcpp::Named("components") = to_numeric_matrix(model->get_components()),
        Rcpp::Named("explained_variance") = to_numeric_vector(model->get_explained_variance()),
        Rcpp::Named("explained_variance_ratio") = to_numeric_vector(model->get_explained_variance_ratio()),
        Rcpp::Named("singular_values") = to_numeric_vector(model->get_singular_values()),
        Rcpp::Named("mean") = to_numeric_vector(model->get_mean()),
        Rcpp::Named("scale") = to_numeric_vector(model->get_scale()),
        Rcpp::Named("n_components") = model->get_n_components()
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_knn(SEXP matrixSEXP, SEXP kSEXP, SEXP metricSEXP) {
    BEGIN_RCPP

    auto* model = new dimensionality_reduction::KNN(Rcpp::as<int>(kSEXP), Rcpp::as<std::string>(metricSEXP));
    model->fit(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    return Rcpp::XPtr<dimensionality_reduction::KNN>(model, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_knn_kneighbors(SEXP ptrSEXP, SEXP matrixSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::KNN> model(ptrSEXP);
    std::pair<mytrix::MatrixI, mytrix::Matrix> result;
    if (Rf_isNull(matrixSEXP)) {
        result = model->kneighbors();
    } else {
        result = model->kneighbors(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    }

    return Rcpp::List::create(
        Rcpp::Named("indices") = to_integer_matrix(result.first, true),
        Rcpp::Named("distances") = to_numeric_matrix(result.second)
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_knn_pairwise_distances(SEXP ptrSEXP, SEXP xSEXP, SEXP ySEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::KNN> model(ptrSEXP);
    return to_numeric_matrix(model->pairwise_distances(
        to_eigen_matrix(Rcpp::NumericMatrix(xSEXP)),
        to_eigen_matrix(Rcpp::NumericMatrix(ySEXP))
    ));

    END_RCPP
}

extern "C" SEXP _coolboxr_fit_umap(SEXP matrixSEXP, SEXP componentsSEXP, SEXP neighborsSEXP, SEXP minDistSEXP, SEXP metricSEXP, SEXP learningRateSEXP, SEXP epochsSEXP, SEXP randomStateSEXP) {
    BEGIN_RCPP

    auto* model = new dimensionality_reduction::UMAP(
        Rcpp::as<int>(componentsSEXP),
        Rcpp::as<int>(neighborsSEXP),
        Rcpp::as<double>(minDistSEXP),
        Rcpp::as<std::string>(metricSEXP),
        Rcpp::as<double>(learningRateSEXP),
        Rcpp::as<int>(epochsSEXP),
        Rcpp::as<int>(randomStateSEXP)
    );
    model->fit(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP)));
    return Rcpp::XPtr<dimensionality_reduction::UMAP>(model, true);

    END_RCPP
}

extern "C" SEXP _coolboxr_umap_transform(SEXP ptrSEXP, SEXP matrixSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::UMAP> model(ptrSEXP);
    return to_numeric_matrix(model->transform(to_eigen_matrix(Rcpp::NumericMatrix(matrixSEXP))));

    END_RCPP
}

extern "C" SEXP _coolboxr_umap_embedding(SEXP ptrSEXP) {
    BEGIN_RCPP

    Rcpp::XPtr<dimensionality_reduction::UMAP> model(ptrSEXP);
    return to_numeric_matrix(model->get_embedding());

    END_RCPP
}
