#include "trace_opto.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace agents {

void TraceGraph::check(NodeId id) const {
    if (id >= nodes_.size()) {
        throw std::out_of_range("TraceGraph: unknown node");
    }
}

NodeId TraceGraph::parameter(const std::string& name, double value) {
    Record record;
    record.name = name;
    record.value = value;
    record.is_parameter = true;
    nodes_.push_back(std::move(record));
    return nodes_.size() - 1;
}

NodeId TraceGraph::constant(const std::string& name, double value) {
    Record record;
    record.name = name;
    record.value = value;
    nodes_.push_back(std::move(record));
    return nodes_.size() - 1;
}

NodeId TraceGraph::apply(const std::string& op,
                         const std::vector<NodeId>& inputs,
                         std::function<double(const std::vector<double>&)> fn) {
    if (!fn) {
        throw std::invalid_argument("TraceGraph::apply: an operation body is required");
    }
    for (NodeId input : inputs) {
        check(input);
    }

    Record record;
    record.name = "v" + std::to_string(nodes_.size());
    record.op = op;
    record.parents = inputs;
    record.fn = std::move(fn);
    nodes_.push_back(std::move(record));
    return nodes_.size() - 1;
}

NodeId TraceGraph::add(NodeId a, NodeId b) {
    return apply("add", {a, b}, [](const std::vector<double>& v) { return v[0] + v[1]; });
}

NodeId TraceGraph::subtract(NodeId a, NodeId b) {
    return apply("subtract", {a, b}, [](const std::vector<double>& v) { return v[0] - v[1]; });
}

NodeId TraceGraph::multiply(NodeId a, NodeId b) {
    return apply("multiply", {a, b}, [](const std::vector<double>& v) { return v[0] * v[1]; });
}

NodeId TraceGraph::square(NodeId a) {
    return apply("square", {a}, [](const std::vector<double>& v) { return v[0] * v[0]; });
}

double TraceGraph::forward(NodeId output) {
    check(output);
    // Parents always have smaller ids, so id order is already topological.
    for (size_t id = 0; id <= output; ++id) {
        Record& record = nodes_[id];
        if (!record.fn) {
            continue;
        }
        std::vector<double> inputs;
        inputs.reserve(record.parents.size());
        for (NodeId parent : record.parents) {
            inputs.push_back(nodes_[parent].value);
        }
        record.value = record.fn(inputs);
    }
    return nodes_[output].value;
}

double TraceGraph::value(NodeId id) const {
    check(id);
    return nodes_[id].value;
}

void TraceGraph::set_parameter(NodeId id, double value) {
    check(id);
    if (!nodes_[id].is_parameter) {
        throw std::invalid_argument("TraceGraph::set_parameter: node is not a parameter");
    }
    nodes_[id].value = value;
}

double TraceGraph::parameter_value(NodeId id) const {
    check(id);
    if (!nodes_[id].is_parameter) {
        throw std::invalid_argument("TraceGraph::parameter_value: node is not a parameter");
    }
    return nodes_[id].value;
}

std::vector<NodeId> TraceGraph::parameters() const {
    std::vector<NodeId> ids;
    for (size_t id = 0; id < nodes_.size(); ++id) {
        if (nodes_[id].is_parameter) {
            ids.push_back(id);
        }
    }
    return ids;
}

TraceSubgraph TraceGraph::trace(NodeId output) const {
    check(output);
    std::vector<char> reachable(nodes_.size(), 0);
    reachable[output] = 1;
    // Walk backwards; parents always precede their children.
    for (size_t id = output + 1; id-- > 0;) {
        if (!reachable[id]) {
            continue;
        }
        for (NodeId parent : nodes_[id].parents) {
            reachable[parent] = 1;
        }
    }

    TraceSubgraph subgraph;
    subgraph.output = output;
    for (size_t id = 0; id < nodes_.size(); ++id) {
        if (!reachable[id]) {
            continue;
        }
        const Record& record = nodes_[id];
        subgraph.nodes.push_back(TraceNode{id, record.name, record.op, record.parents,
                                           record.value, record.is_parameter});
        if (record.is_parameter) {
            subgraph.parameters.push_back(id);
        }
    }
    return subgraph;
}

std::string TraceSubgraph::render() const {
    std::map<NodeId, std::string> names;
    for (const TraceNode& node : nodes) {
        names[node.id] = node.name;
    }

    std::ostringstream out;
    out << "#Parameters\n";
    for (const TraceNode& node : nodes) {
        if (node.is_parameter) {
            out << node.name << " = " << node.value << "\n";
        }
    }
    out << "#Constants\n";
    for (const TraceNode& node : nodes) {
        if (!node.is_parameter && node.op.empty()) {
            out << node.name << " = " << node.value << "\n";
        }
    }
    out << "#Code\n";
    for (const TraceNode& node : nodes) {
        if (node.op.empty()) {
            continue;
        }
        out << node.name << " = " << node.op << "(";
        for (size_t i = 0; i < node.parents.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            out << names[node.parents[i]];
        }
        out << ")\n";
    }
    out << "#Output\n";
    for (const TraceNode& node : nodes) {
        if (node.id == output) {
            out << node.name << " = " << node.value << "\n";
            break;
        }
    }
    return out.str();
}

// ----------------------------------------------------------------------------
// OptoPrime
// ----------------------------------------------------------------------------

OptoPrime::OptoPrime() : OptoPrime(Config()) {}

OptoPrime::OptoPrime(Config config) : config_(config) {
    if (config_.initial_step <= 0.0) {
        throw std::invalid_argument("OptoPrime: initial_step must be positive");
    }
    if (config_.growth < 1.0) {
        throw std::invalid_argument("OptoPrime: growth must be at least 1");
    }
    if (config_.shrink <= 0.0 || config_.shrink >= 1.0) {
        throw std::invalid_argument("OptoPrime: shrink must be in (0, 1)");
    }
}

bool OptoPrime::improved(double score) const {
    return config_.minimize ? score < best_score_ : score > best_score_;
}

void OptoPrime::remember(const std::vector<double>& assignment, double score) {
    memory_.emplace_back(assignment, score);
    if (memory_.size() > config_.memory_size) {
        memory_.erase(memory_.begin());
    }
}

void OptoPrime::step(TraceGraph& graph, const TraceSubgraph& trace, const Feedback& feedback) {
    // The trace is what restricts the search: parameters that did not
    // contribute to the output are never touched.
    if (!initialized_) {
        tracked_ = trace.parameters;
        states_.assign(tracked_.size(), ParameterState{config_.initial_step, 1.0});
        best_assignment_.resize(tracked_.size());
        for (size_t i = 0; i < tracked_.size(); ++i) {
            best_assignment_[i] = graph.parameter_value(tracked_[i]);
        }
        best_score_ = feedback.score;
        initialized_ = true;
        remember(best_assignment_, feedback.score);

        if (!tracked_.empty()) {
            cursor_ = 0;
            last_moved_ = 0;
            graph.set_parameter(tracked_[0], best_assignment_[0] + states_[0].step);
        }
        ++iterations_;
        return;
    }
    if (tracked_.empty()) {
        ++iterations_;
        return;
    }

    std::vector<double> current(tracked_.size());
    for (size_t i = 0; i < tracked_.size(); ++i) {
        current[i] = graph.parameter_value(tracked_[i]);
    }
    remember(current, feedback.score);

    if (improved(feedback.score)) {
        best_score_ = feedback.score;
        best_assignment_ = current;
        states_[last_moved_].step *= config_.growth;
    } else {
        for (size_t i = 0; i < tracked_.size(); ++i) {
            graph.set_parameter(tracked_[i], best_assignment_[i]);
        }
        states_[last_moved_].direction = -states_[last_moved_].direction;
        states_[last_moved_].step *= config_.shrink;
    }

    cursor_ = (cursor_ + 1) % tracked_.size();
    last_moved_ = cursor_;
    const ParameterState& state = states_[cursor_];
    graph.set_parameter(tracked_[cursor_], best_assignment_[cursor_] + state.direction * state.step);
    ++iterations_;
}

double optimize(TraceGraph& graph,
                NodeId output,
                OptoOptimizer& optimizer,
                size_t iterations,
                const std::function<Feedback(double)>& make_feedback) {
    if (!make_feedback) {
        throw std::invalid_argument("optimize: a feedback function is required");
    }
    double best = 0.0;
    for (size_t i = 0; i < iterations; ++i) {
        const double value = graph.forward(output);
        const Feedback feedback = make_feedback(value);
        if (i == 0) {
            best = feedback.score;
        }
        best = std::min(best, feedback.score);
        optimizer.step(graph, graph.trace(output), feedback);
    }
    return best;
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
