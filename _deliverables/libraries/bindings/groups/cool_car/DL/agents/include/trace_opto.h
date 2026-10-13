#ifndef ML_DEEP_LEARNING_AGENTS_TRACE_OPTO_H
#define ML_DEEP_LEARNING_AGENTS_TRACE_OPTO_H

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// @brief Optimization with a Trace Oracle (OPTO).
///
/// Implements Cheng et al., "Trace is the Next AutoDiff: Generative
/// Optimization with Rich Feedback, Execution Traces, and LLMs", NeurIPS 2024
/// (arXiv:2406.16218), citation key @c cheng_trace_2024.
///
/// A workflow is recorded as a DAG of operations over parameter and
/// intermediate nodes. Rather than back-propagating a gradient, the optimizer
/// is handed the execution trace of the output together with feedback on it,
/// and proposes new parameter values. The trace plays the role the gradient
/// plays in AutoDiff: it tells the optimizer which parameters could possibly
/// have caused the observed output.

using NodeId = size_t;

/// A node as seen by the optimizer.
struct TraceNode {
    NodeId id = 0;
    std::string name;
    std::string op;                 ///< Empty for parameters and constants.
    std::vector<NodeId> parents;
    double value = 0.0;
    bool is_parameter = false;
};

/// The execution trace of one output: every node that contributed to it.
struct TraceSubgraph {
    NodeId output = 0;
    std::vector<TraceNode> nodes;       ///< Topologically ordered.
    std::vector<NodeId> parameters;     ///< Parameters the output actually depends on.

    /// Human- and LLM-readable rendering of the trace, in the spirit of the
    /// representation Trace hands to a generative optimizer.
    std::string render() const;
};

/// Feedback on a computed output. It is deliberately richer than a gradient and
/// weaker than one: a score plus free-form text, with no derivative anywhere.
struct Feedback {
    double score = 0.0;
    std::string text;
};

/// A recorded computational workflow.
class TraceGraph {
public:
    NodeId parameter(const std::string& name, double value);
    NodeId constant(const std::string& name, double value);

    /// Registers an arbitrary, possibly non-differentiable, operation.
    NodeId apply(const std::string& op,
                 const std::vector<NodeId>& inputs,
                 std::function<double(const std::vector<double>&)> fn);

    NodeId add(NodeId a, NodeId b);
    NodeId subtract(NodeId a, NodeId b);
    NodeId multiply(NodeId a, NodeId b);
    NodeId square(NodeId a);

    /// Evaluates the workflow and returns the value at @p output.
    double forward(NodeId output);

    double value(NodeId id) const;
    void set_parameter(NodeId id, double value);
    double parameter_value(NodeId id) const;
    std::vector<NodeId> parameters() const;
    size_t size() const { return nodes_.size(); }

    /// The trace oracle: everything that influenced @p output.
    TraceSubgraph trace(NodeId output) const;

private:
    struct Record {
        std::string name;
        std::string op;
        std::vector<NodeId> parents;
        std::function<double(const std::vector<double>&)> fn;
        double value = 0.0;
        bool is_parameter = false;
    };

    void check(NodeId id) const;

    std::vector<Record> nodes_;
};

/// Consumes a trace plus feedback and proposes new parameter values.
class OptoOptimizer {
public:
    virtual ~OptoOptimizer() = default;
    virtual void step(TraceGraph& graph, const TraceSubgraph& trace, const Feedback& feedback) = 0;
    virtual std::string name() const = 0;
};

/// A deterministic OPTO optimizer.
///
/// It stands in for the paper's LLM-backed OptoPrime while keeping the same
/// contract: it never sees a derivative, only the trace and the feedback score.
/// The trace tells it which parameters are worth touching; a per-parameter
/// adaptive probe, plus a memory of past attempts, does the rest. This is
/// enough to perform the first-order numerical optimization the paper
/// demonstrates OptoPrime is capable of.
class OptoPrime : public OptoOptimizer {
public:
    struct Config {
        double initial_step = 0.5;
        double growth = 1.5;   ///< Step multiplier after an improvement.
        double shrink = 0.5;   ///< Step multiplier after a regression.
        size_t memory_size = 64;
        bool minimize = true;
    };

    OptoPrime();
    explicit OptoPrime(Config config);

    void step(TraceGraph& graph, const TraceSubgraph& trace, const Feedback& feedback) override;
    std::string name() const override { return "OptoPrime"; }

    size_t iterations() const { return iterations_; }
    double best_score() const { return best_score_; }
    /// Past (parameter assignment, score) pairs, most recent last.
    const std::vector<std::pair<std::vector<double>, double>>& memory() const { return memory_; }
    /// Parameters the optimizer has decided it is allowed to move.
    const std::vector<NodeId>& tracked_parameters() const { return tracked_; }

private:
    struct ParameterState {
        double step = 0.0;
        double direction = 1.0;
    };

    bool improved(double score) const;
    void remember(const std::vector<double>& assignment, double score);

    Config config_;
    std::vector<NodeId> tracked_;
    std::vector<ParameterState> states_;
    std::vector<double> best_assignment_;
    double best_score_ = 0.0;
    size_t cursor_ = 0;
    size_t last_moved_ = 0;
    size_t iterations_ = 0;
    bool initialized_ = false;
    std::vector<std::pair<std::vector<double>, double>> memory_;
};

/// Runs an OPTO loop: evaluate, hand the trace and feedback to the optimizer,
/// repeat. Returns the best score seen.
double optimize(TraceGraph& graph,
                NodeId output,
                OptoOptimizer& optimizer,
                size_t iterations,
                const std::function<Feedback(double)>& make_feedback);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_TRACE_OPTO_H
