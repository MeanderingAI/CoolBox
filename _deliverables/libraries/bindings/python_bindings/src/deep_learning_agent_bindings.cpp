#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "agents.h"

namespace py = pybind11;

void bind_deep_learning_agents(py::module_& parent) {
    using namespace ml::deep_learning::agents;

    py::module_ module = parent.def_submodule("agents", "LLM agent research primitives");

    py::class_<TraceNode>(module, "TraceNode")
        .def_readonly("id", &TraceNode::id)
        .def_readonly("name", &TraceNode::name)
        .def_readonly("op", &TraceNode::op)
        .def_readonly("parents", &TraceNode::parents)
        .def_readonly("value", &TraceNode::value)
        .def_readonly("is_parameter", &TraceNode::is_parameter);
    py::class_<TraceSubgraph>(module, "TraceSubgraph")
        .def_readonly("output", &TraceSubgraph::output)
        .def_readonly("nodes", &TraceSubgraph::nodes)
        .def_readonly("parameters", &TraceSubgraph::parameters)
        .def("render", &TraceSubgraph::render);
    py::class_<Feedback>(module, "Feedback")
        .def(py::init<>())
        .def_readwrite("score", &Feedback::score)
        .def_readwrite("text", &Feedback::text);
    py::class_<TraceGraph>(module, "TraceGraph")
        .def(py::init<>())
        .def("parameter", &TraceGraph::parameter)
        .def("constant", &TraceGraph::constant)
        .def("apply", &TraceGraph::apply)
        .def("add", &TraceGraph::add)
        .def("subtract", &TraceGraph::subtract)
        .def("multiply", &TraceGraph::multiply)
        .def("square", &TraceGraph::square)
        .def("forward", &TraceGraph::forward)
        .def("value", &TraceGraph::value)
        .def("set_parameter", &TraceGraph::set_parameter)
        .def("parameter_value", &TraceGraph::parameter_value)
        .def_property_readonly("parameters", &TraceGraph::parameters)
        .def("__len__", &TraceGraph::size)
        .def("trace", &TraceGraph::trace);
    py::class_<OptoOptimizer, std::shared_ptr<OptoOptimizer>>(module, "OptoOptimizer");
    py::class_<OptoPrime::Config>(module, "OptoPrimeConfig")
        .def(py::init<>())
        .def_readwrite("initial_step", &OptoPrime::Config::initial_step)
        .def_readwrite("growth", &OptoPrime::Config::growth)
        .def_readwrite("shrink", &OptoPrime::Config::shrink)
        .def_readwrite("memory_size", &OptoPrime::Config::memory_size)
        .def_readwrite("minimize", &OptoPrime::Config::minimize);
    py::class_<OptoPrime, OptoOptimizer, std::shared_ptr<OptoPrime>>(module, "OptoPrime")
        .def(py::init<>())
        .def(py::init<OptoPrime::Config>())
        .def("step", &OptoPrime::step)
        .def_property_readonly("iterations", &OptoPrime::iterations)
        .def_property_readonly("best_score", &OptoPrime::best_score)
        .def_property_readonly("memory", &OptoPrime::memory)
        .def_property_readonly("tracked_parameters", &OptoPrime::tracked_parameters);
    module.def("optimize_trace", &optimize);

    py::class_<ReflexionStep>(module, "ReflexionStep")
        .def(py::init<>())
        .def_readwrite("action", &ReflexionStep::action)
        .def_readwrite("observation", &ReflexionStep::observation);
    py::class_<ReflexionTrajectory>(module, "ReflexionTrajectory")
        .def(py::init<>())
        .def_readwrite("steps", &ReflexionTrajectory::steps)
        .def_readwrite("output", &ReflexionTrajectory::output);
    py::class_<ReflexionEvaluation>(module, "ReflexionEvaluation")
        .def(py::init<>())
        .def_readwrite("reward", &ReflexionEvaluation::reward)
        .def_readwrite("passed", &ReflexionEvaluation::passed)
        .def_readwrite("feedback", &ReflexionEvaluation::feedback);
    py::class_<ReflexionTrial>(module, "ReflexionTrial")
        .def_readonly("trajectory", &ReflexionTrial::trajectory)
        .def_readonly("evaluation", &ReflexionTrial::evaluation)
        .def_readonly("reflection", &ReflexionTrial::reflection);
    py::class_<ReflexionResult>(module, "ReflexionResult")
        .def_readonly("passed", &ReflexionResult::passed)
        .def_readonly("trials", &ReflexionResult::trials)
        .def_readonly("memory", &ReflexionResult::memory);
    py::class_<ReflexionAgent::Config>(module, "ReflexionConfig")
        .def(py::init<>())
        .def_readwrite("max_trials", &ReflexionAgent::Config::max_trials)
        .def_readwrite("memory_capacity", &ReflexionAgent::Config::memory_capacity);
    py::class_<ReflexionAgent>(module, "ReflexionAgent")
        .def(py::init<ReflexionActor, ReflexionEvaluator, ReflexionSelfReflection>())
        .def(py::init<ReflexionActor, ReflexionEvaluator, ReflexionSelfReflection,
                      ReflexionAgent::Config>())
        .def("run", &ReflexionAgent::run)
        .def_property_readonly("memory", &ReflexionAgent::memory)
        .def("clear_memory", &ReflexionAgent::clear_memory);
    py::enum_<ReflexionFailureCause>(module, "ReflexionFailureCause")
        .value("NONE", ReflexionFailureCause::none)
        .value("REPEATED_CYCLE", ReflexionFailureCause::repeated_cycle)
        .value("ACTION_LIMIT", ReflexionFailureCause::action_limit);
    module.def("detect_alfworld_failure", &detect_alfworld_failure,
               py::arg("trajectory"), py::arg("repeated_cycle_limit") = 3,
               py::arg("action_limit") = 30);
    module.def("evaluate_alfworld_trajectory", &evaluate_alfworld_trajectory,
               py::arg("trajectory"), py::arg("environment_success"),
               py::arg("repeated_cycle_limit") = 3, py::arg("action_limit") = 30);

    py::class_<BapoTask>(module, "BapoTask")
        .def(py::init<>())
        .def_readwrite("name", &BapoTask::name)
        .def_readwrite("length", &BapoTask::length)
        .def_readwrite("alphabet_size", &BapoTask::alphabet_size)
        .def_readwrite("evaluate", &BapoTask::evaluate)
        .def_property_readonly("input_space_size", &BapoTask::input_space_size);
    py::class_<BapoBandwidth>(module, "BapoBandwidth")
        .def(py::init<>())
        .def_readwrite("prefix_bits", &BapoBandwidth::prefix_bits)
        .def_readwrite("attention_tokens", &BapoBandwidth::attention_tokens);
    py::class_<BapoOracles>(module, "BapoOracles")
        .def(py::init<>())
        .def_readwrite("prefix_oracle", &BapoOracles::prefix_oracle)
        .def_readwrite("attention_oracle", &BapoOracles::attention_oracle)
        .def_readwrite("decoder", &BapoOracles::decoder);
    py::class_<BapoMachine>(module, "BapoMachine")
        .def(py::init<BapoBandwidth, BapoOracles>())
        .def_property_readonly("bandwidth", &BapoMachine::bandwidth)
        .def("run", &BapoMachine::run)
        .def("solves", &BapoMachine::solves);
    py::class_<BapoAnalyzer>(module, "BapoAnalyzer")
        .def(py::init<BapoTask, size_t>(),
             py::arg("task"), py::arg("max_input_space") = 1u << 22)
        .def_property_readonly("task", &BapoAnalyzer::task)
        .def("prefix_classes", &BapoAnalyzer::prefix_classes)
        .def("minimum_prefix_bits", &BapoAnalyzer::minimum_prefix_bits)
        .def("minimum_prefix_bits_with_attention",
             &BapoAnalyzer::minimum_prefix_bits_with_attention)
        .def("bandwidth", &BapoAnalyzer::bandwidth,
             py::arg("attention_tokens") = 0)
        .def("hardest_split", &BapoAnalyzer::hardest_split,
             py::arg("attention_tokens") = 0)
        .def("is_bapo_easy", &BapoAnalyzer::is_bapo_easy,
             py::arg("budget_bits"), py::arg("attention_tokens") = 0);
    py::class_<CotDecomposition>(module, "CotDecomposition")
        .def(py::init<>())
        .def_readwrite("name", &CotDecomposition::name)
        .def_readwrite("steps", &CotDecomposition::steps);
    module.def("cot_bandwidth", &cot_bandwidth,
               py::arg("decomposition"), py::arg("attention_tokens") = 0);
    module.def("make_index_task", &make_index_task);
    module.def("make_majority_task", &make_majority_task);
    module.def("make_equality_task", &make_equality_task);
    module.def("make_set_disjointness_task", &make_set_disjointness_task);
    module.def("make_two_sum_task", &make_two_sum_task);
    module.def("make_reachability_task", &make_reachability_task);
    module.def("make_parity_task", &make_parity_task);
    module.def("make_equality_cot", &make_equality_cot);

    py::class_<SteeringSample>(module, "SteeringSample")
        .def(py::init<>())
        .def_readwrite("source", &SteeringSample::source)
        .def_readwrite("goal", &SteeringSample::goal)
        .def_readwrite("output", &SteeringSample::output);
    py::class_<SteeringDecomposition>(module, "SteeringDecomposition")
        .def_readonly("requested_distance", &SteeringDecomposition::requested_distance)
        .def_readonly("achieved_along_goal",
                      &SteeringDecomposition::achieved_along_goal)
        .def_readonly("miscalibration", &SteeringDecomposition::miscalibration)
        .def_readonly("side_effect_magnitude",
                      &SteeringDecomposition::side_effect_magnitude)
        .def_readonly("steering_error", &SteeringDecomposition::steering_error)
        .def_readonly("per_dimension_side_effect",
                      &SteeringDecomposition::per_dimension_side_effect);
    py::class_<SteerabilityReport>(module, "SteerabilityReport")
        .def_readonly("samples", &SteerabilityReport::samples)
        .def_readonly("mean_steering_error",
                      &SteerabilityReport::mean_steering_error)
        .def_readonly("mean_miscalibration",
                      &SteerabilityReport::mean_miscalibration)
        .def_readonly("mean_side_effect", &SteerabilityReport::mean_side_effect)
        .def_readonly("steerability_index", &SteerabilityReport::steerability_index)
        .def_readonly("undershoot_rate", &SteerabilityReport::undershoot_rate)
        .def_readonly("mean_per_dimension_side_effect",
                      &SteerabilityReport::mean_per_dimension_side_effect);
    py::class_<SteeringSimulator>(module, "SteeringSimulator")
        .def(py::init<>())
        .def_readwrite("gain", &SteeringSimulator::gain)
        .def_readwrite("side_effect_strength",
                       &SteeringSimulator::side_effect_strength)
        .def_readwrite("side_effect_dimension",
                       &SteeringSimulator::side_effect_dimension)
        .def("respond", &SteeringSimulator::respond);
    module.def("decompose_steering", &decompose);
    module.def("evaluate_steerability", &evaluate_steerability);
    module.def("worst_side_effect_dimension", &worst_side_effect_dimension);
    module.def("sample_goals", &sample_goals);
    module.def("best_of_n", &best_of_n);
    module.def("goal_distance", &goal_distance);

    py::class_<LlfHypothesis>(module, "LlfHypothesis")
        .def(py::init<>())
        .def_readwrite("name", &LlfHypothesis::name)
        .def_readwrite("reward", &LlfHypothesis::reward)
        .def_readwrite("feedback", &LlfHypothesis::feedback);
    py::class_<LlfProblem>(module, "LlfProblem")
        .def(py::init<>())
        .def_readwrite("num_actions", &LlfProblem::num_actions)
        .def_readwrite("hypotheses", &LlfProblem::hypotheses)
        .def_readwrite("truth", &LlfProblem::truth)
        .def("true_reward", &LlfProblem::true_reward)
        .def("true_feedback", &LlfProblem::true_feedback)
        .def_property_readonly("optimal_reward", &LlfProblem::optimal_reward);
    module.def("transfer_eluder_dimension", &transfer_eluder_dimension,
               py::arg("problem"), py::arg("epsilon") = 1e-9,
               py::arg("max_actions") = 18);
    py::class_<Helix>(module, "Helix")
        .def(py::init<LlfProblem>())
        .def("select_action", &Helix::select_action)
        .def("observe", &Helix::observe)
        .def("interact", &Helix::interact)
        .def_property_readonly("version_space", &Helix::version_space)
        .def_property_readonly("rounds", &Helix::rounds)
        .def_property_readonly("cumulative_regret", &Helix::cumulative_regret)
        .def_property_readonly("mistakes", &Helix::mistakes)
        .def_property_readonly("identified", &Helix::identified);
    py::class_<MemorylessPrompting>(module, "MemorylessPrompting")
        .def(py::init<LlfProblem>())
        .def("interact", &MemorylessPrompting::interact)
        .def_property_readonly("rounds", &MemorylessPrompting::rounds)
        .def_property_readonly("cumulative_regret",
                               &MemorylessPrompting::cumulative_regret)
        .def_property_readonly("mistakes", &MemorylessPrompting::mistakes);
    module.def("make_reward_only_problem", &make_reward_only_problem);
    module.def("make_rich_feedback_problem", &make_rich_feedback_problem);

    py::class_<PaperCitation>(module, "PaperCitation")
        .def_readonly("key", &PaperCitation::key)
        .def_readonly("title", &PaperCitation::title)
        .def_readonly("authors", &PaperCitation::authors)
        .def_readonly("venue", &PaperCitation::venue)
        .def_readonly("arxiv_id", &PaperCitation::arxiv_id)
        .def_readonly("url", &PaperCitation::url)
        .def_readonly("component", &PaperCitation::component);
    module.def("citations", &agent_citations);
    module.def("citations_for_component", &citations_for_component);
    module.def("to_bibtex", &to_bibtex);
}
