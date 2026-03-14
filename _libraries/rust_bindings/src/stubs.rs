fn stub_error(name: &str) -> String {
    format!("{name} is not implemented in the Rust bindings yet")
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SplitCriterion {
    Gini,
    Entropy,
}

pub struct DecisionTree {
    pub criterion: SplitCriterion,
}

impl DecisionTree {
    pub fn new() -> Self {
        Self {
            criterion: SplitCriterion::Gini,
        }
    }

    pub fn with_criterion(criterion: SplitCriterion) -> Self {
        Self { criterion }
    }

    pub fn fit(&mut self, _x: &[Vec<i32>], _y: &[i32], _max_depth: i32) -> Result<(), String> {
        Err(stub_error("DecisionTree::fit"))
    }

    pub fn predict(&self, _sample: &[i32]) -> Result<i32, String> {
        Err(stub_error("DecisionTree::predict"))
    }
}

impl Default for DecisionTree {
    fn default() -> Self {
        Self::new()
    }
}

pub struct RandomForest {
    pub num_trees: i32,
    pub max_depth: i32,
}

impl RandomForest {
    pub fn new(num_trees: i32, max_depth: i32) -> Self {
        Self {
            num_trees,
            max_depth,
        }
    }

    pub fn fit(&mut self, _x: &[Vec<i32>], _y: &[i32]) -> Result<(), String> {
        Err(stub_error("RandomForest::fit"))
    }

    pub fn predict(&self, _sample: &[i32]) -> Result<i32, String> {
        Err(stub_error("RandomForest::predict"))
    }
}

#[derive(Debug, Clone, Copy)]
pub struct BoostTreeParameters {
    pub num_estimators: u32,
    pub learning_rate: f64,
    pub max_depth: u32,
}

impl Default for BoostTreeParameters {
    fn default() -> Self {
        Self {
            num_estimators: 100,
            learning_rate: 0.1,
            max_depth: 3,
        }
    }
}

pub struct BoostTree {
    pub params: BoostTreeParameters,
}

impl BoostTree {
    pub fn new(params: BoostTreeParameters) -> Self {
        Self { params }
    }

    pub fn fit(&mut self, _x: &[Vec<f64>], _y: &[f64]) -> Result<(), String> {
        Err(stub_error("BoostTree::fit"))
    }

    pub fn predict(&self, _sample: &[f64]) -> Result<f64, String> {
        Err(stub_error("BoostTree::predict"))
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum KernelType {
    Linear,
    Rbf { gamma: f64 },
    Polynomial {
        gamma: f64,
        coef0: f64,
        degree: i32,
    },
}

impl Default for KernelType {
    fn default() -> Self {
        Self::Linear
    }
}

pub struct Svm {
    pub kernel: KernelType,
}

impl Svm {
    pub fn new(kernel: KernelType) -> Self {
        Self { kernel }
    }

    pub fn fit(&mut self, _x: &[Vec<f64>], _y: &[f64]) -> Result<(), String> {
        Err(stub_error("Svm::fit"))
    }

    pub fn predict(&self, _sample: &[f64]) -> Result<f64, String> {
        Err(stub_error("Svm::predict"))
    }
}

pub struct Pca {
    pub n_components: i32,
    pub center: bool,
    pub scale: bool,
}

impl Pca {
    pub fn new(n_components: i32, center: bool, scale: bool) -> Self {
        Self {
            n_components,
            center,
            scale,
        }
    }

    pub fn fit(&mut self, _x: &[Vec<f64>]) -> Result<(), String> {
        Err(stub_error("Pca::fit"))
    }

    pub fn transform(&self, _x: &[Vec<f64>]) -> Result<Vec<Vec<f64>>, String> {
        Err(stub_error("Pca::transform"))
    }
}

pub struct Hmm {
    pub states: i32,
    pub observations: i32,
}

impl Hmm {
    pub fn new(states: i32, observations: i32) -> Self {
        Self {
            states,
            observations,
        }
    }

    pub fn train(&mut self, _sequences: &[Vec<i32>], _max_iter: i32) -> Result<(), String> {
        Err(stub_error("Hmm::train"))
    }

    pub fn get_most_likely_states(&self, _observations: &[i32]) -> Result<Vec<i32>, String> {
        Err(stub_error("Hmm::get_most_likely_states"))
    }

    pub fn log_likelihood(&self, _observations: &[i32]) -> Result<f64, String> {
        Err(stub_error("Hmm::log_likelihood"))
    }
}

pub struct BayesianNetwork;

impl BayesianNetwork {
    pub fn new() -> Self {
        Self
    }

    pub fn add_node(&mut self, _name: &str, _states: &[String]) -> Result<i32, String> {
        Err(stub_error("BayesianNetwork::add_node"))
    }

    pub fn add_edge(&mut self, _parent: i32, _child: i32) -> Result<(), String> {
        Err(stub_error("BayesianNetwork::add_edge"))
    }

    pub fn set_cpt(&mut self, _node_id: i32, _cpt: &[f64]) -> Result<(), String> {
        Err(stub_error("BayesianNetwork::set_cpt"))
    }

    pub fn num_nodes(&self) -> Result<usize, String> {
        Err(stub_error("BayesianNetwork::num_nodes"))
    }

    pub fn get_node_id(&self, _name: &str) -> Result<i32, String> {
        Err(stub_error("BayesianNetwork::get_node_id"))
    }
}

impl Default for BayesianNetwork {
    fn default() -> Self {
        Self::new()
    }
}

pub struct MarkedPointProcess {
    pub num_marks: i32,
    pub learning_rate: f64,
    pub max_iterations: i32,
}

impl MarkedPointProcess {
    pub fn new(num_marks: i32, learning_rate: f64, max_iterations: i32) -> Self {
        Self {
            num_marks,
            learning_rate,
            max_iterations,
        }
    }

    pub fn fit(
        &mut self,
        _event_times: &[Vec<f64>],
        _event_marks: &[Vec<i32>],
    ) -> Result<(), String> {
        Err(stub_error("MarkedPointProcess::fit"))
    }
}

pub struct LatentSentimentAnalysis {
    pub num_features: i32,
}

impl LatentSentimentAnalysis {
    pub fn new(num_features: i32) -> Self {
        Self { num_features }
    }

    pub fn train(&mut self, _dtm: &[Vec<f64>]) -> Result<(), String> {
        Err(stub_error("LatentSentimentAnalysis::train"))
    }

    pub fn predict_score(&self, _doc_index: i32, _term_index: i32) -> Result<f64, String> {
        Err(stub_error("LatentSentimentAnalysis::predict_score"))
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BanditStats {
    pub true_probability: f64,
    pub estimated_probability: f64,
    pub times_pulled: i32,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct SimulationResult {
    pub bandit_results: Vec<BanditStats>,
}

pub struct BanditArm {
    pub true_prob: f64,
}

impl BanditArm {
    pub fn new(true_prob: f64) -> Self {
        Self { true_prob }
    }

    pub fn pull(&mut self) -> Result<f64, String> {
        Err(stub_error("BanditArm::pull"))
    }

    pub fn update(&mut self, _reward: f64) -> Result<(), String> {
        Err(stub_error("BanditArm::update"))
    }

    pub fn get_estimated_prob(&self) -> Result<f64, String> {
        Err(stub_error("BanditArm::get_estimated_prob"))
    }

    pub fn get_pull_count(&self) -> Result<i32, String> {
        Err(stub_error("BanditArm::get_pull_count"))
    }

    pub fn get_true_prob(&self) -> f64 {
        self.true_prob
    }
}

pub struct EpsilonGreedyAgent {
    pub true_probs: Vec<f64>,
    pub epsilon: f64,
    pub seed: i64,
}

impl EpsilonGreedyAgent {
    pub fn new(true_probs: Vec<f64>, epsilon: f64, seed: i64) -> Self {
        Self {
            true_probs,
            epsilon,
            seed,
        }
    }

    pub fn run_simulation(&mut self, _num_steps: i32) -> Result<SimulationResult, String> {
        Err(stub_error("EpsilonGreedyAgent::run_simulation"))
    }
}

pub struct UcbAgent {
    pub true_probs: Vec<f64>,
    pub c: f64,
}

impl UcbAgent {
    pub fn new(true_probs: Vec<f64>, c: f64) -> Self {
        Self { true_probs, c }
    }

    pub fn run_simulation(&mut self, _num_steps: i32) -> Result<SimulationResult, String> {
        Err(stub_error("UcbAgent::run_simulation"))
    }
}

pub struct ThompsonSamplingAgent {
    pub true_probs: Vec<f64>,
    pub seed: i64,
}

impl ThompsonSamplingAgent {
    pub fn new(true_probs: Vec<f64>, seed: i64) -> Self {
        Self { true_probs, seed }
    }

    pub fn run_simulation(&mut self, _num_steps: i32) -> Result<SimulationResult, String> {
        Err(stub_error("ThompsonSamplingAgent::run_simulation"))
    }
}

pub struct DecayingEpsilonGreedyAgent {
    pub true_probs: Vec<f64>,
    pub initial_epsilon: f64,
    pub decay_rate: f64,
    pub seed: i64,
}

impl DecayingEpsilonGreedyAgent {
    pub fn new(true_probs: Vec<f64>, initial_epsilon: f64, decay_rate: f64, seed: i64) -> Self {
        Self {
            true_probs,
            initial_epsilon,
            decay_rate,
            seed,
        }
    }

    pub fn run_simulation(&mut self, _num_steps: i32) -> Result<SimulationResult, String> {
        Err(stub_error("DecayingEpsilonGreedyAgent::run_simulation"))
    }
}
