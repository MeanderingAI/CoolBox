#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

// Include all the ML headers
#include "decision_tree/decision_tree.h"
#include "decision_tree/random_forest.h"
// #include "decision_tree/decision_tree_regressor.h"  // TODO: Implement this class
#include "support_vector_machine/support_vector_machine.h"
#include "support_vector_machine/linear_kernel.h"
#include "support_vector_machine/rbf_kernel.h"
#include "support_vector_machine/polynomial_kernel.h"
#include "support_vector_machine/sigmoid_kernel.h"
#include "bayesian_network/bayesian_network.h"
#include "hidden_markov_model/hidden_markov_model.h"
#include "generalized_linear_model/linear_regression.h"
#include "multi_arm_bandit/bandit_arm.h"
#include "multi_arm_bandit/decaying_epsilon_agent.h"
#include "tracker/kalman_filter.h"
#include "tracker/unscented_kalman_filter.h"
#include "dimensionality_reduction/svd.h"
#include "dimensionality_reduction/pca.h"
#include "dimensionality_reduction/knn.h"
#include "dimensionality_reduction/umap.h"
#include "deep_learning/tensor.h"
#include "deep_learning/layer.h"
#include "deep_learning/loss.h"
#include "deep_learning/optimizer.h"
#include "deep_learning/neural_network.h"
#include "deep_learning/templates.h"
#include "computer_vision/image.h"
#include "computer_vision/transforms.h"
#include "computer_vision/pipeline.h"
#include "computer_vision/layers.h"
#include "time_series/time_series.h"
#include "nlp/text_processor.h"
#include "nlp/embeddings.h"
#include "distributed/message_passing.h"
#include "distributed/distributed_trainer.h"

namespace py = pybind11;

PYBIND11_MODULE(ml_core, m) {
    m.doc() = "Machine Learning Core Library Python Bindings";

    // Decision Tree Module
    py::module_ dt_module = m.def_submodule("decision_tree", "Decision Tree algorithms");
    
    // SplitCriterion enum
    py::enum_<SplitCriterion>(dt_module, "SplitCriterion")
        .value("GINI", SplitCriterion::GINI)
        .value("ENTROPY", SplitCriterion::ENTROPY);

    // DecisionTree class
    py::class_<DecisionTree>(dt_module, "DecisionTree")
        .def(py::init<SplitCriterion>(), py::arg("criterion") = SplitCriterion::GINI)
        .def("fit", &DecisionTree::fit,
             "Train the decision tree",
             py::arg("X"), py::arg("y"), py::arg("max_depth"))
        .def("predict", &DecisionTree::predict,
             "Predict class label for a sample",
             py::arg("sample"));

    // RandomForest class (assuming it exists)
    // py::class_<RandomForest>(dt_module, "RandomForest")
    //     .def(py::init<int, SplitCriterion>(), 
    //          py::arg("n_estimators") = 10, py::arg("criterion") = SplitCriterion::GINI)
    //     .def("fit", &RandomForest::fit)
    //     .def("predict", &RandomForest::predict);

    // Support Vector Machine Module
    py::module_ svm_module = m.def_submodule("svm", "Support Vector Machine algorithms");
    
    // Kernel base class
    py::class_<Kernel>(svm_module, "Kernel");
    
    // Linear Kernel
    py::class_<LinearKernel, Kernel>(svm_module, "LinearKernel")
        .def(py::init<>())
        .def("calculate", &LinearKernel::calculate);
    
    // RBF Kernel
    py::class_<RBFKernel, Kernel>(svm_module, "RBFKernel")
        .def(py::init<double>(), py::arg("gamma"))
        .def("calculate", &RBFKernel::calculate);
    
    // Polynomial Kernel
    py::class_<PolynomialKernel, Kernel>(svm_module, "PolynomialKernel")
        .def(py::init<int, double, double>(), 
             py::arg("degree"), py::arg("gamma"), py::arg("coef0"))
        .def("calculate", &PolynomialKernel::calculate);
    
    // Sigmoid Kernel
    py::class_<SigmoidKernel, Kernel>(svm_module, "SigmoidKernel")
        .def(py::init<double, double>(), py::arg("gamma"), py::arg("coef0"))
        .def("calculate", &SigmoidKernel::calculate);
    
    // SVM class
    py::class_<SVM>(svm_module, "SVM")
        .def(py::init<const Kernel&>(), py::arg("kernel"))
        .def("fit", &SVM::fit,
             "Train the SVM model",
             py::arg("X"), py::arg("y"))
        .def("predict", &SVM::predict,
             "Predict using the trained SVM",
             py::arg("sample"));

    // Bayesian Network Module
    py::module_ bn_module = m.def_submodule("bayesian_network", "Bayesian Network algorithms");
    
    // BayesianNetwork::Node struct
    py::class_<BayesianNetwork::Node>(bn_module, "Node")
        .def(py::init<>())
        .def_readwrite("name", &BayesianNetwork::Node::name)
        .def_readwrite("states", &BayesianNetwork::Node::states)
        .def_readwrite("index", &BayesianNetwork::Node::index);
    
    // BayesianNetwork class
    py::class_<BayesianNetwork>(bn_module, "BayesianNetwork")
        .def(py::init<>())
        .def("add_node", &BayesianNetwork::add_node,
             "Add a new node to the network",
             py::arg("node_name"), py::arg("states"))
        .def("add_edge", &BayesianNetwork::add_edge,
             "Add an edge between two nodes",
             py::arg("parent_index"), py::arg("child_index"))
        .def("set_cpt", &BayesianNetwork::set_cpt,
             "Set conditional probability table for a node",
             py::arg("node_index"), py::arg("cpt"))
        .def("calculate_joint_probability", &BayesianNetwork::calculate_joint_probability,
             "Calculate joint probability of an assignment",
             py::arg("assignment"))
        .def("infer", &BayesianNetwork::infer,
             "Perform probabilistic inference",
             py::arg("query_node_index"), py::arg("query_state_index"), py::arg("evidence"));

    // Hidden Markov Model Module
    py::module_ hmm_module = m.def_submodule("hmm", "Hidden Markov Model algorithms");
    
    py::class_<HMM>(hmm_module, "HMM")
        .def(py::init<int, int>(), py::arg("states"), py::arg("observations"))
        .def("set_initial_probabilities", &HMM::set_initial_probabilities,
             "Set initial state probabilities",
             py::arg("pi"))
        .def("set_transition_matrix", &HMM::set_transition_matrix,
             "Set state transition matrix",
             py::arg("A"))
        .def("set_emission_matrix", &HMM::set_emission_matrix,
             "Set observation emission matrix",
             py::arg("B"))
        .def("get_initial_probabilities", &HMM::get_initial_probabilities,
             "Get initial state probabilities")
        .def("get_transition_matrix", &HMM::get_transition_matrix,
             "Get state transition matrix")
        .def("get_emission_matrix", &HMM::get_emission_matrix,
             "Get observation emission matrix")
        .def("log_likelihood", &HMM::log_likelihood,
             "Calculate log likelihood of observation sequence",
             py::arg("observations"))
        .def("get_most_likely_states", &HMM::get_most_likely_states,
             "Get most likely hidden state sequence (Viterbi algorithm)",
             py::arg("observations"))
        .def("train", &HMM::train,
             "Train HMM using Baum-Welch algorithm",
             py::arg("observation_sequences"), 
             py::arg("max_iterations") = 100,
             py::arg("tolerance") = 1e-6,
             py::arg("smoothing_factor") = 0,
             py::arg("seed") = 0);

    // Generalized Linear Model Module
    py::module_ glm_module = m.def_submodule("glm", "Generalized Linear Model algorithms");
    
    // FitMethod base class
    py::class_<FitMethod>(glm_module, "FitMethod");
    
    // LinearRegressionFitMethod
    py::enum_<LinearRegressionFitMethod::Type>(glm_module, "LinearRegressionType")
        .value("GRADIENT_DESCENT", LinearRegressionFitMethod::Type::GRADIENT_DESCENT)
        .value("CLOSED_FORM", LinearRegressionFitMethod::Type::CLOSED_FORM);
    
    py::class_<LinearRegressionFitMethod, FitMethod>(glm_module, "LinearRegressionFitMethod")
        .def(py::init<unsigned int, double, LinearRegressionFitMethod::Type>(),
             py::arg("num_iterations") = 10000,
             py::arg("learning_rate") = 0.01,
             py::arg("type") = LinearRegressionFitMethod::Type::GRADIENT_DESCENT)
        .def("get_num_iterations", &LinearRegressionFitMethod::get_num_iterations)
        .def("get_learning_rate", &LinearRegressionFitMethod::get_learning_rate)
        .def("get_type", &LinearRegressionFitMethod::get_type);
    
    // GLM base class
    py::class_<GLM>(glm_module, "GLM")
        .def("fit", &GLM::fit,
             "Train the model",
             py::arg("X"), py::arg("y"))
        .def("predict", &GLM::predict,
             "Predict output for a sample",
             py::arg("sample"));
    
    // LinearRegression class
    py::class_<LinearRegression, GLM>(glm_module, "LinearRegression")
        .def(py::init<const LinearRegressionFitMethod&>(), py::arg("fit_method"))
        .def("fit", &LinearRegression::fit,
             "Train the linear regression model",
             py::arg("X"), py::arg("y"))
        .def("predict", &LinearRegression::predict,
             "Predict using the trained model",
             py::arg("sample"))
        .def("get_coefficients", &LinearRegression::get_coefficients,
             "Get learned coefficients (weights and bias)");

    // Multi-arm Bandit Module
    py::module_ mab_module = m.def_submodule("multi_arm_bandit", "Multi-arm Bandit algorithms");
    
    py::class_<BanditArm>(mab_module, "BanditArm")
        .def(py::init<double>(), py::arg("true_reward_prob"))
        .def("pull", &BanditArm::pull,
             "Pull the bandit arm and get reward")
        .def("update", &BanditArm::update,
             "Update arm statistics with reward",
             py::arg("reward"))
        .def("get_estimated_prob", &BanditArm::get_estimated_prob,
             "Get estimated reward probability")
        .def("get_pull_count", &BanditArm::get_pull_count,
             "Get number of times arm was pulled")
        .def("get_true_prob", &BanditArm::get_true_prob,
             "Get true reward probability");

    // Tracker Module (Kalman Filters)
    py::module_ tracker_module = m.def_submodule("tracker", "State estimation and tracking algorithms");
    
    // Note: Kalman Filter bindings would need to be implemented based on the actual interface
    // This is a placeholder - you'll need to check the actual KalmanFilter class interface
    /*
    py::class_<KalmanFilter>(tracker_module, "KalmanFilter")
        .def(py::init<>())
        .def("predict", &KalmanFilter::predict)
        .def("update", &KalmanFilter::update);
    */

    // Dimensionality Reduction Module
    py::module_ dr_module = m.def_submodule("dimensionality_reduction", 
                                             "Dimensionality reduction algorithms (SVD, PCA)");
    
    using namespace dimensionality_reduction;
    
    // SVD class
    py::class_<SVD>(dr_module, "SVD")
        .def(py::init<bool>(), py::arg("compute_full_matrices") = false,
             "Create SVD object\n\n"
             "Args:\n"
             "    compute_full_matrices: If True, compute full U and V matrices")
        .def("compute", &SVD::compute,
             "Compute SVD of matrix X\n\n"
             "Args:\n"
             "    X: Input matrix (m x n)",
             py::arg("X"))
        .def("get_U", &SVD::get_U,
             "Get left singular vectors (U matrix)\n\n"
             "Returns:\n"
             "    U matrix (m x k) where k = min(m,n) for thin SVD")
        .def("get_singular_values", &SVD::get_singular_values,
             "Get singular values as vector\n\n"
             "Returns:\n"
             "    Vector of singular values in descending order")
        .def("get_V", &SVD::get_V,
             "Get right singular vectors (V matrix)\n\n"
             "Returns:\n"
             "    V matrix (n x k) where k = min(m,n) for thin SVD")
        .def("get_S", &SVD::get_S,
             "Get singular values as diagonal matrix\n\n"
             "Returns:\n"
             "    Diagonal matrix of singular values")
        .def("reconstruct", &SVD::reconstruct,
             "Reconstruct matrix from SVD components\n\n"
             "Args:\n"
             "    num_components: Number of components to use (0 = all)\n\n"
             "Returns:\n"
             "    Reconstructed matrix",
             py::arg("num_components") = 0)
        .def("rank", &SVD::rank,
             "Get rank of matrix\n\n"
             "Args:\n"
             "    tolerance: Tolerance for considering singular values as zero\n\n"
             "Returns:\n"
             "    Estimated rank",
             py::arg("tolerance") = -1.0)
        .def("explained_variance_ratio", &SVD::explained_variance_ratio,
             "Get explained variance ratio for each component\n\n"
             "Returns:\n"
             "    Vector of explained variance ratios")
        .def("is_computed", &SVD::is_computed,
             "Check if SVD has been computed\n\n"
             "Returns:\n"
             "    True if compute() has been called");
    
    // PCA class
    py::class_<PCA>(dr_module, "PCA")
        .def(py::init<int, bool, bool>(),
             py::arg("n_components") = 0,
             py::arg("center") = true,
             py::arg("scale") = false,
             "Create PCA object\n\n"
             "Args:\n"
             "    n_components: Number of components to keep (0 = keep all)\n"
             "    center: If True, center data by subtracting mean\n"
             "    scale: If True, scale data to unit variance")
        .def("fit", &PCA::fit,
             "Fit PCA to data matrix X\n\n"
             "Args:\n"
             "    X: Data matrix (rows = samples, cols = features)",
             py::arg("X"))
        .def("transform", &PCA::transform,
             "Transform data to principal component space\n\n"
             "Args:\n"
             "    X: Data matrix to transform\n\n"
             "Returns:\n"
             "    Transformed data (rows = samples, cols = n_components)",
             py::arg("X"))
        .def("fit_transform", &PCA::fit_transform,
             "Fit and transform in one step\n\n"
             "Args:\n"
             "    X: Data matrix\n\n"
             "Returns:\n"
             "    Transformed data",
             py::arg("X"))
        .def("inverse_transform", &PCA::inverse_transform,
             "Inverse transform from PC space back to original space\n\n"
             "Args:\n"
             "    X_transformed: Data in PC space\n\n"
             "Returns:\n"
             "    Reconstructed data in original space",
             py::arg("X_transformed"))
        .def("get_components", &PCA::get_components,
             "Get principal components (loadings)\n\n"
             "Returns:\n"
             "    Matrix where each column is a principal component")
        .def("get_explained_variance", &PCA::get_explained_variance,
             "Get explained variance for each component\n\n"
             "Returns:\n"
             "    Vector of explained variances")
        .def("get_explained_variance_ratio", &PCA::get_explained_variance_ratio,
             "Get explained variance ratio for each component\n\n"
             "Returns:\n"
             "    Vector of explained variance ratios (sum to 1.0)")
        .def("get_singular_values", &PCA::get_singular_values,
             "Get singular values\n\n"
             "Returns:\n"
             "    Vector of singular values")
        .def("get_mean", &PCA::get_mean,
             "Get mean of training data\n\n"
             "Returns:\n"
             "    Mean vector")
        .def("get_scale", &PCA::get_scale,
             "Get standard deviation of training data\n\n"
             "Returns:\n"
             "    Standard deviation vector")
        .def("get_n_components", &PCA::get_n_components,
             "Get number of components\n\n"
             "Returns:\n"
             "    Number of components kept")
        .def("is_fitted", &PCA::is_fitted,
             "Check if PCA has been fitted\n\n"
             "Returns:\n"
             "    True if fit() has been called");
    
    // KNN class
    py::class_<KNN>(dr_module, "KNN")
        .def(py::init<int, const std::string&>(),
             py::arg("k") = 5,
             py::arg("metric") = "euclidean",
             "Create KNN object\n\n"
             "Args:\n"
             "    k: Number of nearest neighbors\n"
             "    metric: Distance metric ('euclidean', 'manhattan', 'cosine')")
        .def("fit", &KNN::fit,
             "Fit KNN with training data\n\n"
             "Args:\n"
             "    X: Training data matrix",
             py::arg("X"))
        .def("kneighbors", 
             py::overload_cast<const Eigen::MatrixXd&>(&KNN::kneighbors, py::const_),
             "Find k-nearest neighbors for query points\n\n"
             "Args:\n"
             "    X_query: Query points\n\n"
             "Returns:\n"
             "    Tuple of (indices, distances) matrices",
             py::arg("X_query"))
        .def("kneighbors",
             py::overload_cast<>(&KNN::kneighbors, py::const_),
             "Find k-nearest neighbors for training data\n\n"
             "Returns:\n"
             "    Tuple of (indices, distances) matrices")
        .def("pairwise_distances", &KNN::pairwise_distances,
             "Compute pairwise distances\n\n"
             "Args:\n"
             "    X: First set of points\n"
             "    Y: Second set of points\n\n"
             "Returns:\n"
             "    Distance matrix",
             py::arg("X"), py::arg("Y"))
        .def("get_k", &KNN::get_k,
             "Get number of neighbors")
        .def("get_metric", &KNN::get_metric,
             "Get distance metric")
        .def("is_fitted", &KNN::is_fitted,
             "Check if KNN has been fitted");
    
    // UMAP class
    py::class_<UMAP>(dr_module, "UMAP")
        .def(py::init<int, int, double, const std::string&, double, int, int>(),
             py::arg("n_components") = 2,
             py::arg("n_neighbors") = 15,
             py::arg("min_dist") = 0.1,
             py::arg("metric") = "euclidean",
             py::arg("learning_rate") = 1.0,
             py::arg("n_epochs") = 200,
             py::arg("random_state") = 42,
             "Create UMAP object\n\n"
             "Args:\n"
             "    n_components: Number of dimensions in embedding\n"
             "    n_neighbors: Number of nearest neighbors\n"
             "    min_dist: Minimum distance between embedded points\n"
             "    metric: Distance metric ('euclidean', 'manhattan', 'cosine')\n"
             "    learning_rate: Learning rate for optimization\n"
             "    n_epochs: Number of optimization epochs\n"
             "    random_state: Random seed for reproducibility")
        .def("fit", &UMAP::fit,
             "Fit UMAP to data\n\n"
             "Args:\n"
             "    X: Input data matrix",
             py::arg("X"))
        .def("transform", &UMAP::transform,
             "Transform data to embedding\n\n"
             "Args:\n"
             "    X: Data to transform\n\n"
             "Returns:\n"
             "    Embedded data",
             py::arg("X"))
        .def("fit_transform", &UMAP::fit_transform,
             "Fit and transform in one step\n\n"
             "Args:\n"
             "    X: Input data\n\n"
             "Returns:\n"
             "    Embedded data",
             py::arg("X"))
        .def("get_embedding", &UMAP::get_embedding,
             "Get the learned embedding\n\n"
             "Returns:\n"
             "    Embedding matrix")
        .def("is_fitted", &UMAP::is_fitted,
             "Check if UMAP has been fitted")
        .def("get_n_components", &UMAP::get_n_components,
             "Get number of components")
        .def("get_n_neighbors", &UMAP::get_n_neighbors,
             "Get number of neighbors");

    // Deep Learning Module
    py::module_ dl_module = m.def_submodule("deep_learning", "Deep Learning neural networks");
    
    // Tensor class
    py::class_<ml::deep_learning::Tensor>(dl_module, "Tensor")
        .def(py::init<const std::vector<size_t>&>())
        .def(py::init<const std::vector<size_t>&, double>())
        .def(py::init<const std::vector<size_t>&, const std::vector<double>&>())
        .def("data", [](ml::deep_learning::Tensor& t) { return t.data(); })
        .def("shape", &ml::deep_learning::Tensor::shape)
        .def("size", &ml::deep_learning::Tensor::size)
        .def("reshape", &ml::deep_learning::Tensor::reshape)
        .def("transpose", &ml::deep_learning::Tensor::transpose)
        .def("fill", &ml::deep_learning::Tensor::fill)
        .def("randomize", &ml::deep_learning::Tensor::randomize,
             py::arg("min") = -1.0, py::arg("max") = 1.0)
        .def("clone", &ml::deep_learning::Tensor::clone)
        .def("matmul", &ml::deep_learning::Tensor::matmul)
        .def("__add__", &ml::deep_learning::Tensor::operator+)
        .def("__sub__", &ml::deep_learning::Tensor::operator-)
        .def("__mul__", [](const ml::deep_learning::Tensor& t, double s) { return t * s; })
        .def("__truediv__", [](const ml::deep_learning::Tensor& t, double s) { return t / s; })
        .def("at", [](ml::deep_learning::Tensor& t, size_t i) { return t.at(i); })
        .def("__repr__", [](const ml::deep_learning::Tensor& t) {
            std::ostringstream oss;
            oss << "Tensor(shape=[";
            for (size_t i = 0; i < t.shape().size(); ++i) {
                oss << t.shape()[i];
                if (i < t.shape().size() - 1) oss << ", ";
            }
            oss << "], size=" << t.size() << ")";
            return oss.str();
        });
    
    // Layer base class
    py::class_<ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::Layer>>(dl_module, "Layer")
        .def("forward", &ml::deep_learning::Layer::forward)
        .def("backward", &ml::deep_learning::Layer::backward)
        .def("name", &ml::deep_learning::Layer::name)
        .def("has_parameters", &ml::deep_learning::Layer::has_parameters);
    
    // DenseLayer
    py::class_<ml::deep_learning::DenseLayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::DenseLayer>>(dl_module, "DenseLayer")
        .def(py::init<size_t, size_t>())
        .def("weights", &ml::deep_learning::DenseLayer::weights)
        .def("bias", &ml::deep_learning::DenseLayer::bias);
    
    // ReLULayer
    py::class_<ml::deep_learning::ReLULayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::ReLULayer>>(dl_module, "ReLULayer")
        .def(py::init<>());
    
    // SigmoidLayer
    py::class_<ml::deep_learning::SigmoidLayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::SigmoidLayer>>(dl_module, "SigmoidLayer")
        .def(py::init<>());
    
    // TanhLayer
    py::class_<ml::deep_learning::TanhLayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::TanhLayer>>(dl_module, "TanhLayer")
        .def(py::init<>());
    
    // SoftmaxLayer
    py::class_<ml::deep_learning::SoftmaxLayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::SoftmaxLayer>>(dl_module, "SoftmaxLayer")
        .def(py::init<>());
    
    // DropoutLayer
    py::class_<ml::deep_learning::DropoutLayer, ml::deep_learning::Layer, std::shared_ptr<ml::deep_learning::DropoutLayer>>(dl_module, "DropoutLayer")
        .def(py::init<double>())
        .def("set_training", &ml::deep_learning::DropoutLayer::set_training);
    
    // Loss base class
    py::class_<ml::deep_learning::Loss, std::shared_ptr<ml::deep_learning::Loss>>(dl_module, "Loss")
        .def("compute", &ml::deep_learning::Loss::compute)
        .def("gradient", &ml::deep_learning::Loss::gradient)
        .def("name", &ml::deep_learning::Loss::name);
    
    // MSELoss
    py::class_<ml::deep_learning::MSELoss, ml::deep_learning::Loss, std::shared_ptr<ml::deep_learning::MSELoss>>(dl_module, "MSELoss")
        .def(py::init<>());
    
    // BCELoss
    py::class_<ml::deep_learning::BCELoss, ml::deep_learning::Loss, std::shared_ptr<ml::deep_learning::BCELoss>>(dl_module, "BCELoss")
        .def(py::init<>());
    
    // CategoricalCrossEntropyLoss
    py::class_<ml::deep_learning::CategoricalCrossEntropyLoss, ml::deep_learning::Loss, std::shared_ptr<ml::deep_learning::CategoricalCrossEntropyLoss>>(dl_module, "CategoricalCrossEntropyLoss")
        .def(py::init<>());
    
    // Optimizer base class
    py::class_<ml::deep_learning::Optimizer, std::shared_ptr<ml::deep_learning::Optimizer>>(dl_module, "Optimizer")
        .def("step", &ml::deep_learning::Optimizer::step)
        .def("name", &ml::deep_learning::Optimizer::name)
        .def("reset", &ml::deep_learning::Optimizer::reset);
    
    // SGD
    py::class_<ml::deep_learning::SGD, ml::deep_learning::Optimizer, std::shared_ptr<ml::deep_learning::SGD>>(dl_module, "SGD")
        .def(py::init<double, double>(), py::arg("learning_rate"), py::arg("momentum") = 0.0);
    
    // Adam
    py::class_<ml::deep_learning::Adam, ml::deep_learning::Optimizer, std::shared_ptr<ml::deep_learning::Adam>>(dl_module, "Adam")
        .def(py::init<double, double, double, double>(),
             py::arg("learning_rate") = 0.001,
             py::arg("beta1") = 0.9,
             py::arg("beta2") = 0.999,
             py::arg("epsilon") = 1e-8);
    
    // RMSprop
    py::class_<ml::deep_learning::RMSprop, ml::deep_learning::Optimizer, std::shared_ptr<ml::deep_learning::RMSprop>>(dl_module, "RMSprop")
        .def(py::init<double, double, double>(),
             py::arg("learning_rate") = 0.001,
             py::arg("decay") = 0.9,
             py::arg("epsilon") = 1e-8);
    
    // NeuralNetwork
    py::class_<ml::deep_learning::NeuralNetwork>(dl_module, "NeuralNetwork")
        .def(py::init<>())
        .def("add_layer", &ml::deep_learning::NeuralNetwork::add_layer)
        .def("set_loss", &ml::deep_learning::NeuralNetwork::set_loss)
        .def("set_optimizer", &ml::deep_learning::NeuralNetwork::set_optimizer)
        .def("predict", &ml::deep_learning::NeuralNetwork::predict)
        .def("train_step", &ml::deep_learning::NeuralNetwork::train_step)
        .def("train", &ml::deep_learning::NeuralNetwork::train,
             py::arg("inputs"),
             py::arg("targets"),
             py::arg("epochs") = 100,
             py::arg("verbose") = true);
    
    // =========================================================================
    // NEURAL NETWORK TEMPLATES
    // =========================================================================
    
    // CNN Architecture enum
    py::enum_<ml::deep_learning::CNNTemplate::Architecture>(dl_module, "CNNArchitecture")
        .value("SIMPLE", ml::deep_learning::CNNTemplate::Architecture::SIMPLE)
        .value("LENET", ml::deep_learning::CNNTemplate::Architecture::LENET)
        .value("VGGLIKE", ml::deep_learning::CNNTemplate::Architecture::VGGLIKE)
        .value("RESNET", ml::deep_learning::CNNTemplate::Architecture::RESNET);
    
    // RNN Cell Type enum
    py::enum_<ml::deep_learning::RNNTemplate::CellType>(dl_module, "RNNCellType")
        .value("VANILLA", ml::deep_learning::RNNTemplate::CellType::VANILLA)
        .value("LSTM", ml::deep_learning::RNNTemplate::CellType::LSTM)
        .value("GRU", ml::deep_learning::RNNTemplate::CellType::GRU);
    
    // NetworkTemplate base class
    py::class_<ml::deep_learning::NetworkTemplate>(dl_module, "NetworkTemplate")
        .def("build", &ml::deep_learning::NetworkTemplate::build)
        .def("name", &ml::deep_learning::NetworkTemplate::name);
    
    // MLPTemplate
    py::class_<ml::deep_learning::MLPTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "MLPTemplate")
        .def(py::init<int, std::vector<int>, int, std::string, double, bool>(),
             py::arg("input_dim"),
             py::arg("hidden_dims"),
             py::arg("output_dim"),
             py::arg("activation") = "relu",
             py::arg("dropout_rate") = 0.0,
             py::arg("batch_norm") = false,
             "Multi-Layer Perceptron template\n\n"
             "Args:\n"
             "    input_dim: Input dimension\n"
             "    hidden_dims: List of hidden layer sizes\n"
             "    output_dim: Output dimension\n"
             "    activation: Activation function ('relu', 'tanh', 'sigmoid')\n"
             "    dropout_rate: Dropout rate (0 to disable)\n"
             "    batch_norm: Whether to use batch normalization");
    
    // CNNTemplate
    py::class_<ml::deep_learning::CNNTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "CNNTemplate")
        .def(py::init<ml::deep_learning::CNNTemplate::Architecture, int, int, int, int>(),
             py::arg("architecture"),
             py::arg("num_classes"),
             py::arg("input_channels") = 3,
             py::arg("input_height") = 32,
             py::arg("input_width") = 32,
             "Convolutional Neural Network template\n\n"
             "Args:\n"
             "    architecture: Network architecture (SIMPLE, LENET, VGGLIKE, RESNET)\n"
             "    num_classes: Number of output classes\n"
             "    input_channels: Number of input channels\n"
             "    input_height: Input image height\n"
             "    input_width: Input image width");
    
    // AutoencoderTemplate
    py::class_<ml::deep_learning::AutoencoderTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "AutoencoderTemplate")
        .def(py::init<int, std::vector<int>, int, bool>(),
             py::arg("input_dim"),
             py::arg("encoder_dims"),
             py::arg("latent_dim"),
             py::arg("variational") = false,
             "Autoencoder template\n\n"
             "Args:\n"
             "    input_dim: Input dimension\n"
             "    encoder_dims: Encoder hidden layer sizes\n"
             "    latent_dim: Latent space dimension\n"
             "    variational: Whether to use variational autoencoder")
        .def("build_encoder", &ml::deep_learning::AutoencoderTemplate::build_encoder,
             "Build encoder network only")
        .def("build_decoder", &ml::deep_learning::AutoencoderTemplate::build_decoder,
             "Build decoder network only");
    
    // RNNTemplate
    py::class_<ml::deep_learning::RNNTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "RNNTemplate")
        .def(py::init<int, int, int, int, ml::deep_learning::RNNTemplate::CellType, bool, double>(),
             py::arg("input_dim"),
             py::arg("hidden_dim"),
             py::arg("num_layers"),
             py::arg("output_dim"),
             py::arg("cell_type") = ml::deep_learning::RNNTemplate::CellType::LSTM,
             py::arg("bidirectional") = false,
             py::arg("dropout") = 0.0,
             "Recurrent Neural Network template\n\n"
             "Args:\n"
             "    input_dim: Input dimension\n"
             "    hidden_dim: Hidden state dimension\n"
             "    num_layers: Number of recurrent layers\n"
             "    output_dim: Output dimension\n"
             "    cell_type: Type of RNN cell (VANILLA, LSTM, GRU)\n"
             "    bidirectional: Whether to use bidirectional RNN\n"
             "    dropout: Dropout rate between layers");
    
    // SiameseTemplate
    py::class_<ml::deep_learning::SiameseTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "SiameseTemplate")
        .def(py::init<int, std::vector<int>, int, std::string>(),
             py::arg("input_dim"),
             py::arg("hidden_dims"),
             py::arg("embedding_dim"),
             py::arg("distance_metric") = "euclidean",
             "Siamese Network template for similarity learning\n\n"
             "Args:\n"
             "    input_dim: Input dimension\n"
             "    hidden_dims: Hidden layer sizes\n"
             "    embedding_dim: Embedding dimension\n"
             "    distance_metric: Distance metric ('euclidean', 'cosine')")
        .def("build_embedding_network", &ml::deep_learning::SiameseTemplate::build_embedding_network,
             "Build the shared embedding network");
    
    // GANTemplate
    py::class_<ml::deep_learning::GANTemplate, ml::deep_learning::NetworkTemplate>(dl_module, "GANTemplate")
        .def(py::init<int, int, std::vector<int>, std::vector<int>>(),
             py::arg("latent_dim"),
             py::arg("output_dim"),
             py::arg("generator_dims"),
             py::arg("discriminator_dims"),
             "Generative Adversarial Network template\n\n"
             "Args:\n"
             "    latent_dim: Latent space dimension\n"
             "    output_dim: Output dimension\n"
             "    generator_dims: Generator hidden layer sizes\n"
             "    discriminator_dims: Discriminator hidden layer sizes")
        .def("build_generator", &ml::deep_learning::GANTemplate::build_generator,
             "Build generator network")
        .def("build_discriminator", &ml::deep_learning::GANTemplate::build_discriminator,
             "Build discriminator network");
    
    // Quick builder functions
    dl_module.def("binary_classifier", &ml::deep_learning::templates::binary_classifier,
                 py::arg("input_dim"),
                 py::arg("hidden_dims") = std::vector<int>{64, 32},
                 "Create a binary classification network\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    hidden_dims: Hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Configured neural network with sigmoid output and BCE loss");
    
    dl_module.def("multiclass_classifier", &ml::deep_learning::templates::multiclass_classifier,
                 py::arg("input_dim"),
                 py::arg("num_classes"),
                 py::arg("hidden_dims") = std::vector<int>{128, 64},
                 "Create a multi-class classification network\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    num_classes: Number of classes\n"
                 "    hidden_dims: Hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Configured neural network with softmax output");
    
    dl_module.def("image_classifier", &ml::deep_learning::templates::image_classifier,
                 py::arg("num_classes"),
                 py::arg("channels") = 3,
                 py::arg("height") = 32,
                 py::arg("width") = 32,
                 py::arg("arch") = "simple",
                 "Create an image classification network\n\n"
                 "Args:\n"
                 "    num_classes: Number of classes\n"
                 "    channels: Number of image channels\n"
                 "    height: Image height\n"
                 "    width: Image width\n"
                 "    arch: Architecture ('simple', 'lenet', 'vgg', 'resnet')\n\n"
                 "Returns:\n"
                 "    CNN for image classification");
    
    dl_module.def("regressor", &ml::deep_learning::templates::regressor,
                 py::arg("input_dim"),
                 py::arg("output_dim") = 1,
                 py::arg("hidden_dims") = std::vector<int>{64, 32},
                 "Create a regression network\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    output_dim: Output dimension\n"
                 "    hidden_dims: Hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Configured neural network with MSE loss");
    
    dl_module.def("embedding_network", &ml::deep_learning::templates::embedding_network,
                 py::arg("input_dim"),
                 py::arg("embedding_dim"),
                 py::arg("hidden_dims") = std::vector<int>{128, 64},
                 "Create an embedding network\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    embedding_dim: Embedding dimension\n"
                 "    hidden_dims: Hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Network for learning embeddings");
    
    dl_module.def("sequence_classifier", &ml::deep_learning::templates::sequence_classifier,
                 py::arg("input_dim"),
                 py::arg("num_classes"),
                 py::arg("hidden_dim") = 128,
                 py::arg("num_layers") = 2,
                 "Create a sequence classification network\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension per timestep\n"
                 "    num_classes: Number of classes\n"
                 "    hidden_dim: RNN hidden dimension\n"
                 "    num_layers: Number of RNN layers\n\n"
                 "Returns:\n"
                 "    LSTM-based sequence classifier");
    
    dl_module.def("simple_autoencoder", &ml::deep_learning::templates::simple_autoencoder,
                 py::arg("input_dim"),
                 py::arg("latent_dim"),
                 py::arg("hidden_dims") = std::vector<int>{128, 64},
                 "Create a simple autoencoder\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    latent_dim: Latent space dimension\n"
                 "    hidden_dims: Encoder hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Autoencoder network");
    
    dl_module.def("variational_autoencoder", &ml::deep_learning::templates::variational_autoencoder,
                 py::arg("input_dim"),
                 py::arg("latent_dim"),
                 py::arg("encoder_dims") = std::vector<int>{256, 128},
                 "Create a variational autoencoder\n\n"
                 "Args:\n"
                 "    input_dim: Input dimension\n"
                 "    latent_dim: Latent space dimension\n"
                 "    encoder_dims: Encoder hidden layer sizes\n\n"
                 "Returns:\n"
                 "    VAE network");
    
    dl_module.def("simple_gan", &ml::deep_learning::templates::simple_gan,
                 py::arg("latent_dim"),
                 py::arg("output_dim"),
                 py::arg("generator_dims") = std::vector<int>{128, 256},
                 py::arg("discriminator_dims") = std::vector<int>{256, 128},
                 "Create a simple GAN generator\n\n"
                 "Args:\n"
                 "    latent_dim: Latent noise dimension\n"
                 "    output_dim: Output dimension\n"
                 "    generator_dims: Generator hidden layer sizes\n"
                 "    discriminator_dims: Discriminator hidden layer sizes\n\n"
                 "Returns:\n"
                 "    Generator network");
    
    // =========================================================================
    // COMPUTER VISION MODULE
    // =========================================================================
             py::arg("epochs"),
             py::arg("batch_size") = 32,
             py::arg("verbose") = true);

    // Computer Vision Module
    py::module_ cv_module = m.def_submodule("computer_vision", "Computer Vision algorithms");
    
    // ImageFormat enum
    py::enum_<ml::cv::ImageFormat>(cv_module, "ImageFormat")
        .value("GRAYSCALE", ml::cv::ImageFormat::GRAYSCALE)
        .value("RGB", ml::cv::ImageFormat::RGB)
        .value("RGBA", ml::cv::ImageFormat::RGBA);
    
    // InterpolationMode enum
    py::enum_<ml::cv::InterpolationMode>(cv_module, "InterpolationMode")
        .value("NEAREST", ml::cv::InterpolationMode::NEAREST)
        .value("BILINEAR", ml::cv::InterpolationMode::BILINEAR)
        .value("BICUBIC", ml::cv::InterpolationMode::BICUBIC);
    
    // Image class
    py::class_<ml::cv::Image>(cv_module, "Image")
        .def(py::init<>())
        .def(py::init<int, int, ml::cv::ImageFormat>(),
             py::arg("height"), py::arg("width"), 
             py::arg("format") = ml::cv::ImageFormat::RGB)
        .def(py::init<int, int, int, const std::vector<float>&>(),
             py::arg("height"), py::arg("width"), py::arg("channels"), py::arg("data"))
        .def("height", &ml::cv::Image::height)
        .def("width", &ml::cv::Image::width)
        .def("channels", &ml::cv::Image::channels)
        .def("format", &ml::cv::Image::format)
        .def("size", &ml::cv::Image::size)
        .def("data", py::overload_cast<>(&ml::cv::Image::data))
        .def("at", py::overload_cast<int, int, int>(&ml::cv::Image::at),
             py::arg("row"), py::arg("col"), py::arg("channel") = 0,
             py::return_value_policy::reference)
        .def("clone", &ml::cv::Image::clone)
        .def("fill", py::overload_cast<float>(&ml::cv::Image::fill))
        .def("fill", py::overload_cast<const std::vector<float>&>(&ml::cv::Image::fill))
        .def("to_grayscale", &ml::cv::Image::to_grayscale)
        .def("to_rgb", &ml::cv::Image::to_rgb)
        .def("mean", &ml::cv::Image::mean)
        .def("std", &ml::cv::Image::std)
        .def("min_max", &ml::cv::Image::min_max);
    
    // Transform base class
    py::class_<ml::cv::Transform, std::shared_ptr<ml::cv::Transform>>(cv_module, "Transform")
        .def("apply", &ml::cv::Transform::apply);
    
    // Resize transform
    py::class_<ml::cv::Resize, ml::cv::Transform, std::shared_ptr<ml::cv::Resize>>(cv_module, "Resize")
        .def(py::init<int, int, ml::cv::InterpolationMode>(),
             py::arg("height"), py::arg("width"),
             py::arg("mode") = ml::cv::InterpolationMode::BILINEAR);
    
    // CenterCrop transform
    py::class_<ml::cv::CenterCrop, ml::cv::Transform, std::shared_ptr<ml::cv::CenterCrop>>(cv_module, "CenterCrop")
        .def(py::init<int, int>(), py::arg("height"), py::arg("width"));
    
    // RandomCrop transform
    py::class_<ml::cv::RandomCrop, ml::cv::Transform, std::shared_ptr<ml::cv::RandomCrop>>(cv_module, "RandomCrop")
        .def(py::init<int, int, unsigned int>(),
             py::arg("height"), py::arg("width"), py::arg("seed") = 0);
    
    // HorizontalFlip transform
    py::class_<ml::cv::HorizontalFlip, ml::cv::Transform, std::shared_ptr<ml::cv::HorizontalFlip>>(cv_module, "HorizontalFlip")
        .def(py::init<>());
    
    // VerticalFlip transform
    py::class_<ml::cv::VerticalFlip, ml::cv::Transform, std::shared_ptr<ml::cv::VerticalFlip>>(cv_module, "VerticalFlip")
        .def(py::init<>());
    
    // RandomHorizontalFlip transform
    py::class_<ml::cv::RandomHorizontalFlip, ml::cv::Transform, std::shared_ptr<ml::cv::RandomHorizontalFlip>>(cv_module, "RandomHorizontalFlip")
        .def(py::init<float, unsigned int>(),
             py::arg("probability") = 0.5f, py::arg("seed") = 0);
    
    // Normalize transform
    py::class_<ml::cv::Normalize, ml::cv::Transform, std::shared_ptr<ml::cv::Normalize>>(cv_module, "Normalize")
        .def(py::init<const std::vector<float>&, const std::vector<float>&>(),
             py::arg("mean"), py::arg("std"));
    
    // Standardize transform
    py::class_<ml::cv::Standardize, ml::cv::Transform, std::shared_ptr<ml::cv::Standardize>>(cv_module, "Standardize")
        .def(py::init<>());
    
    // Rotate transform
    py::class_<ml::cv::Rotate, ml::cv::Transform, std::shared_ptr<ml::cv::Rotate>>(cv_module, "Rotate")
        .def(py::init<float, ml::cv::InterpolationMode>(),
             py::arg("angle_degrees"),
             py::arg("mode") = ml::cv::InterpolationMode::BILINEAR);
    
    // RandomRotation transform
    py::class_<ml::cv::RandomRotation, ml::cv::Transform, std::shared_ptr<ml::cv::RandomRotation>>(cv_module, "RandomRotation")
        .def(py::init<float, float, ml::cv::InterpolationMode, unsigned int>(),
             py::arg("min_angle"), py::arg("max_angle"),
             py::arg("mode") = ml::cv::InterpolationMode::BILINEAR,
             py::arg("seed") = 0);
    
    // AdjustBrightness transform
    py::class_<ml::cv::AdjustBrightness, ml::cv::Transform, std::shared_ptr<ml::cv::AdjustBrightness>>(cv_module, "AdjustBrightness")
        .def(py::init<float>(), py::arg("factor"));
    
    // AdjustContrast transform
    py::class_<ml::cv::AdjustContrast, ml::cv::Transform, std::shared_ptr<ml::cv::AdjustContrast>>(cv_module, "AdjustContrast")
        .def(py::init<float>(), py::arg("factor"));
    
    // GaussianBlur transform
    py::class_<ml::cv::GaussianBlur, ml::cv::Transform, std::shared_ptr<ml::cv::GaussianBlur>>(cv_module, "GaussianBlur")
        .def(py::init<int, float>(), py::arg("kernel_size"), py::arg("sigma"));
    
    // Pad transform
    py::class_<ml::cv::Pad, ml::cv::Transform, std::shared_ptr<ml::cv::Pad>>(cv_module, "Pad")
        .def(py::init<int, int, int, int, float>(),
             py::arg("top"), py::arg("bottom"), py::arg("left"), py::arg("right"),
             py::arg("fill_value") = 0.0f);
    
    // TransformPipeline class
    py::class_<ml::cv::TransformPipeline>(cv_module, "TransformPipeline")
        .def(py::init<>())
        .def("add", [](ml::cv::TransformPipeline& self, std::shared_ptr<ml::cv::Transform> transform) {
            self.add(std::unique_ptr<ml::cv::Transform>(transform->clone()));
        })
        .def("apply", &ml::cv::TransformPipeline::apply)
        .def("apply_batch", &ml::cv::TransformPipeline::apply_batch)
        .def("size", &ml::cv::TransformPipeline::size)
        .def("clear", &ml::cv::TransformPipeline::clear)
        .def("clone", &ml::cv::TransformPipeline::clone);
    
    // Predefined pipelines
    cv_module.def("create_imagenet_pipeline", &ml::cv::create_imagenet_pipeline,
                  py::arg("image_size") = 224);
    cv_module.def("create_training_augmentation_pipeline", &ml::cv::create_training_augmentation_pipeline,
                  py::arg("image_size"),
                  py::arg("random_flip") = true,
                  py::arg("random_rotation") = true,
                  py::arg("random_brightness") = true,
                  py::arg("random_contrast") = true);
    cv_module.def("create_inference_pipeline", &ml::cv::create_inference_pipeline,
                  py::arg("image_size"),
                  py::arg("mean") = std::vector<float>{0.485f, 0.456f, 0.406f},
                  py::arg("std") = std::vector<float>{0.229f, 0.224f, 0.225f});
    
    // Utility functions
    cv_module.def("image_to_tensor", &ml::cv::image_to_tensor);
    cv_module.def("tensor_to_image", &ml::cv::tensor_to_image,
                  py::arg("tensor"),
                  py::arg("format") = ml::cv::ImageFormat::RGB);
    
    // CV Layers
    py::class_<ml::cv::Conv2DLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::Conv2DLayer>>(cv_module, "Conv2DLayer")
        .def(py::init<int, int, int, int, int>(),
             py::arg("in_channels"), py::arg("out_channels"), py::arg("kernel_size"),
             py::arg("stride") = 1, py::arg("padding") = 0)
        .def("in_channels", &ml::cv::Conv2DLayer::in_channels)
        .def("out_channels", &ml::cv::Conv2DLayer::out_channels)
        .def("kernel_size", &ml::cv::Conv2DLayer::kernel_size)
        .def("stride", &ml::cv::Conv2DLayer::stride)
        .def("padding", &ml::cv::Conv2DLayer::padding);
    
    py::class_<ml::cv::MaxPool2DLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::MaxPool2DLayer>>(cv_module, "MaxPool2DLayer")
        .def(py::init<int, int>(), py::arg("kernel_size"), py::arg("stride") = -1)
        .def("kernel_size", &ml::cv::MaxPool2DLayer::kernel_size)
        .def("stride", &ml::cv::MaxPool2DLayer::stride);
    
    py::class_<ml::cv::AvgPool2DLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::AvgPool2DLayer>>(cv_module, "AvgPool2DLayer")
        .def(py::init<int, int>(), py::arg("kernel_size"), py::arg("stride") = -1)
        .def("kernel_size", &ml::cv::AvgPool2DLayer::kernel_size)
        .def("stride", &ml::cv::AvgPool2DLayer::stride);
    
    py::class_<ml::cv::BatchNorm2DLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::BatchNorm2DLayer>>(cv_module, "BatchNorm2DLayer")
        .def(py::init<int, float, float>(),
             py::arg("num_features"), py::arg("eps") = 1e-5f, py::arg("momentum") = 0.1f)
        .def("set_training", &ml::cv::BatchNorm2DLayer::set_training)
        .def("is_training", &ml::cv::BatchNorm2DLayer::is_training);
    
    py::class_<ml::cv::FlattenLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::FlattenLayer>>(cv_module, "FlattenLayer")
        .def(py::init<>());
    
    py::class_<ml::cv::GlobalAvgPool2DLayer, ml::deep_learning::Layer, std::shared_ptr<ml::cv::GlobalAvgPool2DLayer>>(cv_module, "GlobalAvgPool2DLayer")
        .def(py::init<>());
    
    // ========== Time Series Module ==========
    py::module_ ts_module = m.def_submodule("time_series", "Time Series Analysis");
    
    // TimeSeries class
    py::class_<ml::time_series::TimeSeries>(ts_module, "TimeSeries")
        .def(py::init<>())
        .def(py::init<const std::vector<double>&, const std::vector<std::string>&>(),
             py::arg("values"), py::arg("timestamps") = std::vector<std::string>())
        .def("size", &ml::time_series::TimeSeries::size)
        .def("values", py::overload_cast<>(&ml::time_series::TimeSeries::values, py::const_))
        .def("at", py::overload_cast<size_t>(&ml::time_series::TimeSeries::at, py::const_))
        .def("timestamp_at", &ml::time_series::TimeSeries::timestamp_at)
        .def("mean", &ml::time_series::TimeSeries::mean)
        .def("std", &ml::time_series::TimeSeries::std)
        .def("min", &ml::time_series::TimeSeries::min)
        .def("max", &ml::time_series::TimeSeries::max)
        .def("median", &ml::time_series::TimeSeries::median)
        .def("normalize", &ml::time_series::TimeSeries::normalize)
        .def("min_max_scale", &ml::time_series::TimeSeries::min_max_scale,
             py::arg("min_val") = 0.0, py::arg("max_val") = 1.0)
        .def("diff", &ml::time_series::TimeSeries::diff, py::arg("lag") = 1)
        .def("log_transform", &ml::time_series::TimeSeries::log_transform)
        .def("moving_average", &ml::time_series::TimeSeries::moving_average)
        .def("exponential_smoothing", &ml::time_series::TimeSeries::exponential_smoothing)
        .def("resample", &ml::time_series::TimeSeries::resample)
        .def("create_windows", &ml::time_series::TimeSeries::create_windows,
             py::arg("window_size"), py::arg("stride") = 1)
        .def("create_supervised_windows", &ml::time_series::TimeSeries::create_supervised_windows,
             py::arg("input_window"), py::arg("output_window") = 1, py::arg("stride") = 1)
        .def("autocorrelation", &ml::time_series::TimeSeries::autocorrelation);
    
    // MultivariateTimeSeries class
    py::class_<ml::time_series::MultivariatTimeSeries>(ts_module, "MultivariatTimeSeries")
        .def(py::init<>())
        .def(py::init<const std::vector<std::vector<double>>&, const std::vector<std::string>&, const std::vector<std::string>&>(),
             py::arg("data"), py::arg("feature_names") = std::vector<std::string>(),
             py::arg("timestamps") = std::vector<std::string>())
        .def("num_samples", &ml::time_series::MultivariatTimeSeries::num_samples)
        .def("num_features", &ml::time_series::MultivariatTimeSeries::num_features)
        .def("data", &ml::time_series::MultivariatTimeSeries::data)
        .def("feature", &ml::time_series::MultivariatTimeSeries::feature)
        .def("sample", &ml::time_series::MultivariatTimeSeries::sample)
        .def("at", &ml::time_series::MultivariatTimeSeries::at)
        .def("means", &ml::time_series::MultivariatTimeSeries::means)
        .def("stds", &ml::time_series::MultivariatTimeSeries::stds)
        .def("normalize", &ml::time_series::MultivariatTimeSeries::normalize)
        .def("min_max_scale", &ml::time_series::MultivariatTimeSeries::min_max_scale)
        .def("create_windows", &ml::time_series::MultivariatTimeSeries::create_windows,
             py::arg("window_size"), py::arg("stride") = 1);
    
    // Forecasting models
    py::class_<ml::time_series::MovingAverageForecaster>(ts_module, "MovingAverageForecaster")
        .def(py::init<size_t>(), py::arg("window_size"))
        .def("fit", &ml::time_series::MovingAverageForecaster::fit)
        .def("forecast", &ml::time_series::MovingAverageForecaster::forecast)
        .def("forecast_one_step", &ml::time_series::MovingAverageForecaster::forecast_one_step);
    
    py::class_<ml::time_series::ExponentialSmoothingForecaster>(ts_module, "ExponentialSmoothingForecaster")
        .def(py::init<double, double, double>(),
             py::arg("alpha"), py::arg("beta") = 0.0, py::arg("gamma") = 0.0)
        .def("fit", &ml::time_series::ExponentialSmoothingForecaster::fit)
        .def("forecast", &ml::time_series::ExponentialSmoothingForecaster::forecast);
    
    py::class_<ml::time_series::AutoRegressiveModel>(ts_module, "AutoRegressiveModel")
        .def(py::init<size_t>(), py::arg("order"))
        .def("fit", &ml::time_series::AutoRegressiveModel::fit)
        .def("forecast", &ml::time_series::AutoRegressiveModel::forecast)
        .def("coefficients", &ml::time_series::AutoRegressiveModel::coefficients);
    
    // Seasonal decomposition struct
    py::class_<ml::time_series::SeasonalDecomposition>(ts_module, "SeasonalDecomposition")
        .def_readwrite("trend", &ml::time_series::SeasonalDecomposition::trend)
        .def_readwrite("seasonal", &ml::time_series::SeasonalDecomposition::seasonal)
        .def_readwrite("residual", &ml::time_series::SeasonalDecomposition::residual);
    
    // Utility functions
    ts_module.def("seasonal_decompose", &ml::time_series::seasonal_decompose);
    ts_module.def("detect_outliers_zscore", &ml::time_series::detect_outliers_zscore,
                  py::arg("ts"), py::arg("threshold") = 3.0);
    ts_module.def("detect_outliers_iqr", &ml::time_series::detect_outliers_iqr,
                  py::arg("ts"), py::arg("multiplier") = 1.5);
    ts_module.def("interpolate_missing", &ml::time_series::interpolate_missing);
    
    // ========== NLP Module ==========
    py::module_ nlp_module = m.def_submodule("nlp", "Natural Language Processing");
    
    // TextProcessor class
    py::class_<ml::nlp::TextProcessor>(nlp_module, "TextProcessor")
        .def(py::init<>())
        .def_static("to_lowercase", &ml::nlp::TextProcessor::to_lowercase)
        .def_static("remove_punctuation", &ml::nlp::TextProcessor::remove_punctuation)
        .def_static("remove_numbers", &ml::nlp::TextProcessor::remove_numbers)
        .def_static("remove_extra_whitespace", &ml::nlp::TextProcessor::remove_extra_whitespace)
        .def_static("strip", &ml::nlp::TextProcessor::strip)
        .def_static("tokenize", &ml::nlp::TextProcessor::tokenize,
                   py::arg("text"), py::arg("delimiter") = " ")
        .def_static("word_tokenize", &ml::nlp::TextProcessor::word_tokenize)
        .def_static("sentence_tokenize", &ml::nlp::TextProcessor::sentence_tokenize)
        .def_static("generate_ngrams", &ml::nlp::TextProcessor::generate_ngrams)
        .def_static("stem", &ml::nlp::TextProcessor::stem)
        .def_static("stem_tokens", &ml::nlp::TextProcessor::stem_tokens)
        .def_static("default_stop_words", &ml::nlp::TextProcessor::default_stop_words)
        .def_static("remove_stop_words", &ml::nlp::TextProcessor::remove_stop_words,
                   py::arg("tokens"), py::arg("stop_words") = ml::nlp::TextProcessor::default_stop_words())
        .def("process", &ml::nlp::TextProcessor::process,
             py::arg("text"), py::arg("lowercase") = true, py::arg("remove_punct") = true,
             py::arg("remove_nums") = false, py::arg("remove_stops") = true,
             py::arg("apply_stemming") = false);
    
    // Vocabulary class
    py::class_<ml::nlp::Vocabulary, std::shared_ptr<ml::nlp::Vocabulary>>(nlp_module, "Vocabulary")
        .def(py::init<size_t, size_t>(), py::arg("min_freq") = 1, py::arg("max_size") = 0)
        .def("build", &ml::nlp::Vocabulary::build)
        .def("build_from_texts", &ml::nlp::Vocabulary::build_from_texts)
        .def("add_special_token", &ml::nlp::Vocabulary::add_special_token)
        .def("token_to_index", &ml::nlp::Vocabulary::token_to_index)
        .def("index_to_token", &ml::nlp::Vocabulary::index_to_token)
        .def("contains", &ml::nlp::Vocabulary::contains)
        .def("encode", &ml::nlp::Vocabulary::encode)
        .def("decode", &ml::nlp::Vocabulary::decode)
        .def("size", &ml::nlp::Vocabulary::size)
        .def("frequency", &ml::nlp::Vocabulary::frequency)
        .def_readonly_static("PAD_IDX", &ml::nlp::Vocabulary::PAD_IDX)
        .def_readonly_static("UNK_IDX", &ml::nlp::Vocabulary::UNK_IDX)
        .def_readonly_static("BOS_IDX", &ml::nlp::Vocabulary::BOS_IDX)
        .def_readonly_static("EOS_IDX", &ml::nlp::Vocabulary::EOS_IDX);
    
    // BagOfWords class
    py::class_<ml::nlp::BagOfWords>(nlp_module, "BagOfWords")
        .def(py::init<std::shared_ptr<ml::nlp::Vocabulary>>(),
             py::arg("vocab") = nullptr)
        .def("fit", &ml::nlp::BagOfWords::fit)
        .def("transform", &ml::nlp::BagOfWords::transform)
        .def("transform_batch", &ml::nlp::BagOfWords::transform_batch)
        .def("vocabulary", &ml::nlp::BagOfWords::vocabulary);
    
    // TFIDF class
    py::class_<ml::nlp::TFIDF>(nlp_module, "TFIDF")
        .def(py::init<std::shared_ptr<ml::nlp::Vocabulary>>(),
             py::arg("vocab") = nullptr)
        .def("fit", &ml::nlp::TFIDF::fit)
        .def("transform", &ml::nlp::TFIDF::transform)
        .def("transform_batch", &ml::nlp::TFIDF::transform_batch)
        .def("vocabulary", &ml::nlp::TFIDF::vocabulary);
    
    // SequenceEncoder class
    py::class_<ml::nlp::SequenceEncoder>(nlp_module, "SequenceEncoder")
        .def(py::init<std::shared_ptr<ml::nlp::Vocabulary>, size_t, bool, bool>(),
             py::arg("vocab"), py::arg("max_length") = 0,
             py::arg("padding") = true, py::arg("truncation") = true)
        .def("encode", py::overload_cast<const std::vector<std::string>&>(&ml::nlp::SequenceEncoder::encode, py::const_))
        .def("encode_batch", &ml::nlp::SequenceEncoder::encode_batch)
        .def("decode", &ml::nlp::SequenceEncoder::decode,
             py::arg("indices"), py::arg("skip_special") = true);
    
    // CharacterEncoder class
    py::class_<ml::nlp::CharacterEncoder>(nlp_module, "CharacterEncoder")
        .def(py::init<>())
        .def("fit", &ml::nlp::CharacterEncoder::fit)
        .def("encode", &ml::nlp::CharacterEncoder::encode)
        .def("decode", &ml::nlp::CharacterEncoder::decode)
        .def("vocab_size", &ml::nlp::CharacterEncoder::vocab_size);
    
    // WordEmbedding class
    py::class_<ml::nlp::WordEmbedding>(nlp_module, "WordEmbedding")
        .def(py::init<size_t>(), py::arg("embedding_dim"))
        .def("random_init", &ml::nlp::WordEmbedding::random_init)
        .def("xavier_init", &ml::nlp::WordEmbedding::xavier_init)
        .def("get_embedding", &ml::nlp::WordEmbedding::get_embedding)
        .def("has_word", &ml::nlp::WordEmbedding::has_word)
        .def("get_embeddings", &ml::nlp::WordEmbedding::get_embeddings)
        .def("update_embedding", &ml::nlp::WordEmbedding::update_embedding)
        .def("similarity", &ml::nlp::WordEmbedding::similarity)
        .def("most_similar", &ml::nlp::WordEmbedding::most_similar,
             py::arg("word"), py::arg("top_k") = 10)
        .def("vocab_size", &ml::nlp::WordEmbedding::vocab_size)
        .def("embedding_dim", &ml::nlp::WordEmbedding::embedding_dim);
    
    // OneHotEncoder class
    py::class_<ml::nlp::OneHotEncoder>(nlp_module, "OneHotEncoder")
        .def(py::init<>())
        .def("fit", &ml::nlp::OneHotEncoder::fit)
        .def("encode", &ml::nlp::OneHotEncoder::encode)
        .def("encode_batch", &ml::nlp::OneHotEncoder::encode_batch)
        .def("vocab_size", &ml::nlp::OneHotEncoder::vocab_size);
    
    // NLP utility functions
    nlp_module.def("cosine_similarity", &ml::nlp::cosine_similarity);
    nlp_module.def("jaccard_similarity", &ml::nlp::jaccard_similarity);
    nlp_module.def("levenshtein_distance", &ml::nlp::levenshtein_distance);
    nlp_module.def("create_positional_encoding", &ml::nlp::create_positional_encoding);
    
    // =========================================================================
    // DISTRIBUTED COMPUTING MODULE
    // =========================================================================
    py::module_ dist_module = m.def_submodule("distributed", "Distributed computing for ML");
    
    // MessageType enum
    py::enum_<distributed::MessageType>(dist_module, "MessageType")
        .value("DATA", distributed::MessageType::DATA)
        .value("GRADIENT", distributed::MessageType::GRADIENT)
        .value("PARAMETER", distributed::MessageType::PARAMETER)
        .value("COMMAND", distributed::MessageType::COMMAND)
        .value("RESULT", distributed::MessageType::RESULT)
        .value("HEARTBEAT", distributed::MessageType::HEARTBEAT)
        .value("BARRIER", distributed::MessageType::BARRIER)
        .value("REDUCE", distributed::MessageType::REDUCE);
    
    // CommPattern enum
    py::enum_<distributed::CommPattern>(dist_module, "CommPattern")
        .value("POINT_TO_POINT", distributed::CommPattern::POINT_TO_POINT)
        .value("BROADCAST", distributed::CommPattern::BROADCAST)
        .value("SCATTER", distributed::CommPattern::SCATTER)
        .value("GATHER", distributed::CommPattern::GATHER)
        .value("ALL_REDUCE", distributed::CommPattern::ALL_REDUCE)
        .value("RING_ALL_REDUCE", distributed::CommPattern::RING_ALL_REDUCE);
    
    // ReduceOp enum
    py::enum_<distributed::ReduceOp>(dist_module, "ReduceOp")
        .value("SUM", distributed::ReduceOp::SUM)
        .value("AVERAGE", distributed::ReduceOp::AVERAGE)
        .value("MIN", distributed::ReduceOp::MIN)
        .value("MAX", distributed::ReduceOp::MAX)
        .value("PRODUCT", distributed::ReduceOp::PRODUCT);
    
    // TrainingStrategy enum
    py::enum_<distributed::TrainingStrategy>(dist_module, "TrainingStrategy")
        .value("DATA_PARALLEL", distributed::TrainingStrategy::DATA_PARALLEL)
        .value("MODEL_PARALLEL", distributed::TrainingStrategy::MODEL_PARALLEL)
        .value("PARAMETER_SERVER", distributed::TrainingStrategy::PARAMETER_SERVER)
        .value("DECENTRALIZED", distributed::TrainingStrategy::DECENTRALIZED)
        .value("FEDERATED", distributed::TrainingStrategy::FEDERATED);
    
    // AggregationMethod enum
    py::enum_<distributed::AggregationMethod>(dist_module, "AggregationMethod")
        .value("SYNCHRONOUS", distributed::AggregationMethod::SYNCHRONOUS)
        .value("ASYNCHRONOUS", distributed::AggregationMethod::ASYNCHRONOUS)
        .value("ELASTIC_AVERAGING", distributed::AggregationMethod::ELASTIC_AVERAGING);
    
    // Message class
    py::class_<distributed::Message>(dist_module, "Message")
        .def(py::init<distributed::MessageType, int, int>(),
             py::arg("type") = distributed::MessageType::DATA,
             py::arg("source_rank") = -1,
             py::arg("dest_rank") = -1)
        .def_readwrite("type", &distributed::Message::type)
        .def_readwrite("source_rank", &distributed::Message::source_rank)
        .def_readwrite("dest_rank", &distributed::Message::dest_rank)
        .def_readwrite("data", &distributed::Message::data)
        .def_readwrite("metadata", &distributed::Message::metadata);
    
    // DistributedContext class
    py::class_<distributed::DistributedContext, std::shared_ptr<distributed::DistributedContext>>(
        dist_module, "DistributedContext")
        .def(py::init<int, int>())
        .def("send", &distributed::DistributedContext::send)
        .def("receive", &distributed::DistributedContext::receive,
             py::arg("source_rank") = -1)
        .def("broadcast", &distributed::DistributedContext::broadcast)
        .def("scatter", &distributed::DistributedContext::scatter)
        .def("gather", &distributed::DistributedContext::gather)
        .def("all_reduce", &distributed::DistributedContext::all_reduce)
        .def("ring_all_reduce", &distributed::DistributedContext::ring_all_reduce)
        .def("barrier", &distributed::DistributedContext::barrier)
        .def("rank", &distributed::DistributedContext::rank)
        .def("world_size", &distributed::DistributedContext::world_size)
        .def("is_master", &distributed::DistributedContext::is_master);
    
    // DataPartitioner class
    py::class_<distributed::DataPartitioner>(dist_module, "DataPartitioner")
        .def(py::init<size_t, int>())
        .def("get_partition", &distributed::DataPartitioner::get_partition)
        .def("get_indices", &distributed::DataPartitioner::get_indices)
        .def("partition_size", &distributed::DataPartitioner::partition_size);
    
    // ParameterServer class
    py::class_<distributed::ParameterServer>(dist_module, "ParameterServer")
        .def(py::init<int>())
        .def("set_parameters", &distributed::ParameterServer::set_parameters)
        .def("get_parameters", &distributed::ParameterServer::get_parameters)
        .def("update_parameters", &distributed::ParameterServer::update_parameters)
        .def("accumulate_gradient", &distributed::ParameterServer::accumulate_gradient)
        .def("apply_gradients", &distributed::ParameterServer::apply_gradients)
        .def("clear_gradients", &distributed::ParameterServer::clear_gradients);
    
    // DistributedTrainer base class
    py::class_<distributed::DistributedTrainer, std::shared_ptr<distributed::DistributedTrainer>>(
        dist_module, "DistributedTrainer")
        .def("train_epoch", &distributed::DistributedTrainer::train_epoch)
        .def("get_parameters", &distributed::DistributedTrainer::get_parameters)
        .def("set_parameters", &distributed::DistributedTrainer::set_parameters)
        .def("predict", &distributed::DistributedTrainer::predict)
        .def("synchronize_model", &distributed::DistributedTrainer::synchronize_model)
        .def("aggregate_gradients", &distributed::DistributedTrainer::aggregate_gradients,
             py::arg("local_gradient"),
             py::arg("method") = distributed::AggregationMethod::SYNCHRONOUS);
    
    // DistributedNeuralNetTrainer class
    py::class_<distributed::DistributedNeuralNetTrainer, distributed::DistributedTrainer,
               std::shared_ptr<distributed::DistributedNeuralNetTrainer>>(
        dist_module, "DistributedNeuralNetTrainer")
        .def(py::init<std::shared_ptr<distributed::DistributedContext>,
                      distributed::TrainingStrategy,
                      int, std::vector<int>, int, double>())
        .def("get_local_loss", &distributed::DistributedNeuralNetTrainer::get_local_loss)
        .def("get_global_loss", &distributed::DistributedNeuralNetTrainer::get_global_loss);
    
    // DistributedKMeansTrainer class
    py::class_<distributed::DistributedKMeansTrainer, distributed::DistributedTrainer,
               std::shared_ptr<distributed::DistributedKMeansTrainer>>(
        dist_module, "DistributedKMeansTrainer")
        .def(py::init<std::shared_ptr<distributed::DistributedContext>,
                      int, int>())
        .def("get_centroids", &distributed::DistributedKMeansTrainer::get_centroids);
    
    // Utility functions
    dist_module.def("partition_data", &distributed::utils::partition_data);
    dist_module.def("compute_distributed_accuracy", &distributed::utils::compute_distributed_accuracy);
}
   nlp_module.def("average_embeddings", &ml::nlp::average_embeddings);
    nlp_module.def("max_pooling_embeddings", &ml::nlp::max_pooling_embeddings);
}
