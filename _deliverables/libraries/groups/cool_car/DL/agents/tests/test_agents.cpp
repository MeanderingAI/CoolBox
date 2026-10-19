#include "tyst_framework.hpp"

#include "agents.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ml::deep_learning::agents;

// ============================================================================
// Citations (all five papers)
// ============================================================================

TYST_TEST(AgentCitationsTest, RegistersEveryImplementedPaper) {
    const auto& citations = agent_citations();
    TYST_ASSERT_EQ(citations.size(), 5u);

    const std::vector<std::string> expected_ids = {
        "2303.11366", "2406.16218", "2505.08140", "2505.23816", "2506.10341"
    };
    for (size_t i = 0; i < expected_ids.size(); ++i) {
        TYST_EXPECT_EQ(citations[i].arxiv_id, expected_ids[i]);
        TYST_EXPECT_FALSE(citations[i].component.empty());
        TYST_EXPECT_FALSE(citations[i].authors.empty());
    }
}

// ============================================================================
// Reflexion (arXiv:2303.11366)
// ============================================================================

TYST_TEST(ReflexionTest, VerbalMemoryChangesTheNextTrialAndSolvesTheTask) {
        ReflexionActor actor = [](const std::string&, const ReflexionMemory& memory) {
            ReflexionTrajectory trajectory;
            const bool has_hint = !memory.empty();
            trajectory.steps.push_back(
                {has_hint ? "take mug from desk" : "search drawer",
                 has_hint ? "mug acquired" : "nothing useful"});
            trajectory.output = has_hint ? "done" : "failed";
            return trajectory;
        };
        ReflexionEvaluator evaluator =
            [](const std::string&, const ReflexionTrajectory& trajectory) {
                const bool passed = trajectory.output == "done";
                return ReflexionEvaluation{
                    passed ? 1.0 : 0.0, passed, passed ? "success" : "mug not found"};
            };
        ReflexionSelfReflection reflector =
            [](const std::string&, const ReflexionTrajectory&,
               const ReflexionEvaluation& evaluation, const ReflexionMemory&) {
                return std::string("Next time, take the visible mug. Feedback: ") +
                       evaluation.feedback;
            };

        ReflexionAgent agent(actor, evaluator, reflector, {4, 3});
        const ReflexionResult result = agent.run("Acquire the mug");

        TYST_EXPECT_TRUE(result.passed);
        TYST_ASSERT_EQ(result.trials.size(), 2u);
        TYST_EXPECT_EQ(result.trials[0].trajectory.steps.size(), 1u);
        TYST_EXPECT_FALSE(result.trials[0].reflection.empty());
        TYST_EXPECT_TRUE(result.trials[1].reflection.empty());
        TYST_ASSERT_EQ(result.memory.size(), 1u);
        TYST_EXPECT_NE(result.memory[0].find("visible mug"), std::string::npos);
}

TYST_TEST(ReflexionTest, EpisodicMemoryIsBoundedAndKeepsNewestExperiences) {
        size_t attempt = 0;
        ReflexionAgent agent(
            [&attempt](const std::string&, const ReflexionMemory&) {
                ReflexionTrajectory trajectory;
                trajectory.output = std::to_string(attempt++);
                return trajectory;
            },
            [](const std::string&, const ReflexionTrajectory&) {
                return ReflexionEvaluation{0.0, false, "failed"};
            },
            [](const std::string&, const ReflexionTrajectory& trajectory,
               const ReflexionEvaluation&, const ReflexionMemory&) {
                return std::string("lesson-") + trajectory.output;
            },
            {5, 3});

        const ReflexionResult result = agent.run("unsolved task");
        TYST_EXPECT_FALSE(result.passed);
        TYST_EXPECT_EQ(result.trials.size(), 5u);
        TYST_ASSERT_EQ(result.memory.size(), 3u);
        TYST_EXPECT_EQ(result.memory[0], std::string("lesson-2"));
        TYST_EXPECT_EQ(result.memory[2], std::string("lesson-4"));
}

TYST_TEST(ReflexionTest, AlfWorldHeuristicDetectsPaperFailureModes) {
        ReflexionTrajectory stuck;
        for (size_t i = 0; i < 4; ++i) {
            stuck.steps.push_back({"take pan", "Nothing happens."});
        }
        TYST_EXPECT_EQ(
            detect_alfworld_failure(stuck), ReflexionFailureCause::repeated_cycle);
        const ReflexionEvaluation stuck_evaluation =
            evaluate_alfworld_trajectory(stuck, false);
        TYST_EXPECT_FALSE(stuck_evaluation.passed);
        TYST_EXPECT_NE(stuck_evaluation.feedback.find("stuck"), std::string::npos);

        ReflexionTrajectory long_plan;
        long_plan.steps.resize(31);
        TYST_EXPECT_EQ(
            detect_alfworld_failure(long_plan), ReflexionFailureCause::action_limit);

        const ReflexionEvaluation success =
            evaluate_alfworld_trajectory(long_plan, true);
        TYST_EXPECT_TRUE(success.passed);
        TYST_EXPECT_NEAR(success.reward, 1.0, 1e-12);
}

TYST_TEST(ReflexionTest, RejectsInvalidConfigurationAndEmptyReflection) {
        ReflexionActor actor = [](const std::string&, const ReflexionMemory&) {
            return ReflexionTrajectory{};
        };
        ReflexionEvaluator evaluator =
            [](const std::string&, const ReflexionTrajectory&) {
                return ReflexionEvaluation{};
            };
        ReflexionSelfReflection empty_reflector =
            [](const std::string&, const ReflexionTrajectory&,
               const ReflexionEvaluation&, const ReflexionMemory&) {
                return std::string();
            };

        TYST_EXPECT_THROW(
            (void) ReflexionAgent(actor, evaluator, empty_reflector, {0, 3}),
            std::invalid_argument);
        ReflexionAgent agent(actor, evaluator, empty_reflector, {1, 1});
        TYST_EXPECT_THROW((void) agent.run("task"), std::runtime_error);
}

TYST_TEST(AgentCitationsTest, LooksUpByKeyAndComponent) {
    const PaperCitation* trace = find_citation("cheng_trace_2024");
    TYST_ASSERT_TRUE(trace != nullptr);
    TYST_EXPECT_EQ(trace->component, std::string("trace_opto.h"));
    TYST_EXPECT_TRUE(find_citation("nope") == nullptr);

    TYST_EXPECT_EQ(citations_for_component("bapo.h").size(), 1u);
    TYST_EXPECT_EQ(citations_for_component("missing.h").size(), 0u);
}

TYST_TEST(AgentCitationsTest, RendersBibtex) {
    const std::string bibtex = to_bibtex(agent_citations());
    TYST_EXPECT_NE(bibtex.find("@inproceedings{schnabel_lost_in_transmission_2025"),
                   std::string::npos);
    TYST_EXPECT_NE(bibtex.find("eprint = {2506.10341}"), std::string::npos);
    TYST_EXPECT_NE(bibtex.find("archivePrefix = {arXiv}"), std::string::npos);
}

// ============================================================================
// BAPO (arXiv:2505.08140)
// ============================================================================

TYST_TEST(BapoTest, EqualityIsBapoHard) {
    const BapoTask task = make_equality_task(3);
    TYST_ASSERT_EQ(task.length, 6u);

    const BapoAnalyzer analyzer(task);
    // Every one of the 2^3 prefixes needs its own code word, so the prefix
    // oracle has to carry the entire first half across the boundary.
    TYST_EXPECT_EQ(analyzer.prefix_classes(3), 8u);
    TYST_EXPECT_EQ(analyzer.minimum_prefix_bits(3), 3u);
    TYST_EXPECT_EQ(analyzer.bandwidth(), 3u);
    TYST_EXPECT_EQ(analyzer.hardest_split(), 3u);
    TYST_EXPECT_FALSE(analyzer.is_bapo_easy(2));
}

TYST_TEST(BapoTest, ParityIsBapoEasy) {
    const BapoAnalyzer analyzer(make_parity_task(6));
    // A single bit, the parity of the prefix, is always enough.
    TYST_EXPECT_EQ(analyzer.bandwidth(), 1u);
    TYST_EXPECT_TRUE(analyzer.is_bapo_easy(1));
}

TYST_TEST(BapoTest, MajorityNeedsOnlyACount) {
    const BapoAnalyzer analyzer(make_majority_task(6));
    // The prefix only has to send how many ones it saw, not which ones.
    TYST_EXPECT_EQ(analyzer.bandwidth(), 2u);
    TYST_EXPECT_LT(analyzer.bandwidth(), BapoAnalyzer(make_equality_task(3)).bandwidth());
}

TYST_TEST(BapoTest, SetDisjointnessAndReachabilityNeedRealBandwidth) {
    TYST_EXPECT_GT(BapoAnalyzer(make_set_disjointness_task(3)).bandwidth(), 1u);
    TYST_EXPECT_GT(BapoAnalyzer(make_reachability_task(3)).bandwidth(), 1u);
}

TYST_TEST(BapoTest, AttentionSolvesTheIndexTask) {
    const BapoTask task = make_index_task(4);
    TYST_ASSERT_EQ(task.length, 6u);

    const BapoAnalyzer analyzer(task);
    // Without attention the whole haystack must be summarised.
    TYST_EXPECT_EQ(analyzer.minimum_prefix_bits(4), 4u);
    // One attention token is enough to fetch the needle directly.
    TYST_EXPECT_EQ(analyzer.minimum_prefix_bits_with_attention(4, 1), 0u);
}

TYST_TEST(BapoTest, ChainOfThoughtMakesEqualityEasy) {
    const size_t half = 3;
    const size_t direct = BapoAnalyzer(make_equality_task(half)).bandwidth();
    const size_t decomposed = cot_bandwidth(make_equality_cot(half));

    TYST_EXPECT_EQ(direct, 3u);
    // Writing the per-position comparisons into the context means no step ever
    // needs to move more than one bit across the boundary.
    TYST_EXPECT_EQ(decomposed, 1u);
    TYST_EXPECT_LT(decomposed, direct);
}

TYST_TEST(BapoTest, MachineWithOneBitSolvesParity) {
    const BapoTask task = make_parity_task(6);

    BapoOracles oracles;
    oracles.prefix_oracle = [](const Input& prefix) {
        int parity = 0;
        for (Symbol symbol : prefix) {
            parity ^= (symbol & 1);
        }
        return std::vector<bool>{parity != 0};
    };
    oracles.decoder = [](const std::vector<bool>& bits, const std::vector<Symbol>&,
                         const Input& suffix) {
        int parity = bits.empty() ? 0 : static_cast<int>(bits[0]);
        for (Symbol symbol : suffix) {
            parity ^= (symbol & 1);
        }
        return parity;
    };

    const BapoMachine machine(BapoBandwidth{1, 0}, oracles);
    TYST_EXPECT_TRUE(machine.solves(task, 3));
}

TYST_TEST(BapoTest, MachineWithOneAttentionTokenSolvesIndex) {
    const BapoTask task = make_index_task(4);

    BapoOracles oracles;
    oracles.attention_oracle = [](const Input& suffix) {
        size_t index = 0;
        for (Symbol symbol : suffix) {
            index = index * 2 + static_cast<size_t>(symbol);
        }
        return std::vector<size_t>{index % 4};
    };
    oracles.decoder = [](const std::vector<bool>&, const std::vector<Symbol>& attended,
                         const Input&) {
        return attended.empty() ? 0 : attended[0];
    };

    // No prefix bits at all: attention alone carries the answer.
    const BapoMachine machine(BapoBandwidth{0, 1}, oracles);
    TYST_EXPECT_TRUE(machine.solves(task, 4));
}

TYST_TEST(BapoTest, MachineRejectsOverspending) {
    BapoOracles oracles;
    oracles.prefix_oracle = [](const Input&) { return std::vector<bool>{true, false}; };
    oracles.decoder = [](const std::vector<bool>&, const std::vector<Symbol>&, const Input&) {
        return 0;
    };

    const BapoMachine machine(BapoBandwidth{1, 0}, oracles);
    TYST_EXPECT_THROW(machine.run(Input{0, 1, 0, 1}, 2), std::runtime_error);
}

// ============================================================================
// Steerability (arXiv:2505.23816)
// ============================================================================

TYST_TEST(SteerabilityTest, PerfectSteeringHasNoErrorOrSideEffects) {
    SteeringSample sample;
    sample.source = {0.2, 0.2, 0.2};
    sample.goal = {0.6, 0.2, 0.2};
    sample.output = {0.6, 0.2, 0.2};

    const SteeringDecomposition decomposition = decompose(sample);
    TYST_EXPECT_NEAR(decomposition.steering_error, 0.0, 1e-12);
    TYST_EXPECT_NEAR(decomposition.miscalibration, 0.0, 1e-12);
    TYST_EXPECT_NEAR(decomposition.side_effect_magnitude, 0.0, 1e-12);
}

TYST_TEST(SteerabilityTest, SeparatesUndershootFromOvershoot) {
    const GoalVector source = {0.2, 0.2, 0.2};
    const GoalVector goal = {0.6, 0.2, 0.2};

    SteeringSimulator timid;
    timid.gain = 0.5;
    SteeringSimulator eager;
    eager.gain = 1.5;

    const SteeringDecomposition under = decompose({source, goal, timid.respond(source, goal)});
    const SteeringDecomposition over = decompose({source, goal, eager.respond(source, goal)});

    TYST_EXPECT_NEAR(under.miscalibration, -0.5, 1e-12);
    TYST_EXPECT_NEAR(over.miscalibration, 0.5, 1e-12);
    // Neither model wandered off the requested axis.
    TYST_EXPECT_NEAR(under.side_effect_magnitude, 0.0, 1e-12);
    TYST_EXPECT_NEAR(over.side_effect_magnitude, 0.0, 1e-12);
}

TYST_TEST(SteerabilityTest, DetectsSideEffectsOnUnrequestedAxes) {
    const GoalVector source = {0.2, 0.2, 0.2};
    const GoalVector goal = {0.6, 0.2, 0.2};

    SteeringSimulator leaky;
    leaky.gain = 1.0;
    leaky.side_effect_strength = 0.25;
    leaky.side_effect_dimension = 1;

    const SteeringDecomposition decomposition =
        decompose({source, goal, leaky.respond(source, goal)});

    // Requested movement was 0.4 along axis 0; the leak is 0.25 of that.
    TYST_EXPECT_NEAR(decomposition.achieved_along_goal, 0.4, 1e-12);
    TYST_EXPECT_NEAR(decomposition.miscalibration, 0.0, 1e-12);
    TYST_EXPECT_NEAR(decomposition.side_effect_magnitude, 0.1, 1e-12);
    TYST_EXPECT_NEAR(decomposition.per_dimension_side_effect[1], 0.1, 1e-12);
    TYST_EXPECT_NEAR(decomposition.per_dimension_side_effect[2], 0.0, 1e-12);

    // The request was purely along axis 0, so the leak is attributed to axis 1.
    const SteerabilityReport report =
        evaluate_steerability({{source, goal, leaky.respond(source, goal)}});
    TYST_EXPECT_EQ(worst_side_effect_dimension(report), 1u);
}

TYST_TEST(SteerabilityTest, ReportSummarisesAcrossProbes) {
    const GoalVector source = {0.5, 0.5, 0.5};
    const std::vector<GoalVector> goals = sample_goals(source, 24, 0.2, 7);
    TYST_ASSERT_EQ(goals.size(), 24u);

    SteeringSimulator model;
    model.gain = 0.6;
    model.side_effect_strength = 0.3;
    model.side_effect_dimension = 2;

    std::vector<SteeringSample> samples;
    for (const GoalVector& goal : goals) {
        TYST_EXPECT_GE(goal_distance(goal, source), 0.2);
        samples.push_back({source, goal, model.respond(source, goal)});
    }

    const SteerabilityReport report = evaluate_steerability(samples);
    TYST_EXPECT_EQ(report.samples, 24u);
    // A gain below one means the model consistently stops short.
    TYST_EXPECT_LT(report.mean_miscalibration, 0.0);
    TYST_EXPECT_NEAR(report.undershoot_rate, 1.0, 1e-12);
    TYST_EXPECT_GT(report.mean_side_effect, 0.0);
    // Still better than leaving the text alone, but far from steerable.
    TYST_EXPECT_GT(report.steerability_index, 0.0);
    TYST_EXPECT_LT(report.steerability_index, 1.0);
}

TYST_TEST(SteerabilityTest, IndexIsOneForPerfectAndZeroForInertModels) {
    const GoalVector source = {0.3, 0.3};
    const std::vector<GoalVector> goals = sample_goals(source, 10, 0.1, 3);

    SteeringSimulator perfect;
    SteeringSimulator inert;
    inert.gain = 0.0;

    std::vector<SteeringSample> good;
    std::vector<SteeringSample> lazy;
    for (const GoalVector& goal : goals) {
        good.push_back({source, goal, perfect.respond(source, goal)});
        lazy.push_back({source, goal, inert.respond(source, goal)});
    }

    TYST_EXPECT_NEAR(evaluate_steerability(good).steerability_index, 1.0, 1e-12);
    TYST_EXPECT_NEAR(evaluate_steerability(lazy).steerability_index, 0.0, 1e-12);
}

TYST_TEST(SteerabilityTest, BestOfNPicksTheClosestCandidate) {
    const GoalVector goal = {0.5, 0.5};
    const std::vector<GoalVector> candidates = {{0.1, 0.1}, {0.45, 0.55}, {0.9, 0.2}};
    TYST_EXPECT_EQ(best_of_n(candidates, goal), 1u);
    TYST_EXPECT_THROW(best_of_n(std::vector<GoalVector>{}, goal), std::invalid_argument);
}

TYST_TEST(SteerabilityTest, GoalSamplingRejectsImpossibleRequests) {
    const GoalVector source = {0.5, 0.5};
    TYST_EXPECT_THROW(sample_goals(source, 5, 10.0, 1), std::runtime_error);
}

// ============================================================================
// Trace / OPTO (arXiv:2406.16218)
// ============================================================================

namespace {

/// f(x, y) = (x - 3)^2 + (y + 1)^2, with an unused parameter alongside it.
struct QuadraticWorkflow {
    TraceGraph graph;
    NodeId x = 0;
    NodeId y = 0;
    NodeId unused = 0;
    NodeId output = 0;

    QuadraticWorkflow() {
        x = graph.parameter("x", 0.0);
        y = graph.parameter("y", 0.0);
        unused = graph.parameter("unused", 42.0);
        const NodeId three = graph.constant("three", 3.0);
        const NodeId minus_one = graph.constant("minus_one", -1.0);
        const NodeId dx = graph.subtract(x, three);
        const NodeId dy = graph.subtract(y, minus_one);
        output = graph.add(graph.square(dx), graph.square(dy));
    }
};

} // namespace

TYST_TEST(TraceOptoTest, TraceContainsOnlyContributingParameters) {
    QuadraticWorkflow workflow;
    workflow.graph.forward(workflow.output);

    const TraceSubgraph trace = workflow.graph.trace(workflow.output);
    TYST_ASSERT_EQ(trace.parameters.size(), 2u);
    TYST_EXPECT_EQ(trace.parameters[0], workflow.x);
    TYST_EXPECT_EQ(trace.parameters[1], workflow.y);
    TYST_EXPECT_EQ(trace.output, workflow.output);
}

TYST_TEST(TraceOptoTest, ForwardEvaluatesTheWorkflow) {
    QuadraticWorkflow workflow;
    TYST_EXPECT_NEAR(workflow.graph.forward(workflow.output), 10.0, 1e-12);

    workflow.graph.set_parameter(workflow.x, 3.0);
    workflow.graph.set_parameter(workflow.y, -1.0);
    TYST_EXPECT_NEAR(workflow.graph.forward(workflow.output), 0.0, 1e-12);
}

TYST_TEST(TraceOptoTest, RendersTraceForAGenerativeOptimizer) {
    QuadraticWorkflow workflow;
    workflow.graph.forward(workflow.output);
    const std::string rendered = workflow.graph.trace(workflow.output).render();

    TYST_EXPECT_NE(rendered.find("#Parameters"), std::string::npos);
    TYST_EXPECT_NE(rendered.find("#Code"), std::string::npos);
    TYST_EXPECT_NE(rendered.find("#Output"), std::string::npos);
    TYST_EXPECT_NE(rendered.find("square"), std::string::npos);
    TYST_EXPECT_NE(rendered.find("x = 0"), std::string::npos);
    // The unused parameter never contributed, so it is absent from the trace.
    TYST_EXPECT_EQ(rendered.find("unused"), std::string::npos);
}

TYST_TEST(TraceOptoTest, OptoPrimeMinimisesFromScalarFeedbackAlone) {
    QuadraticWorkflow workflow;
    OptoPrime optimizer;

    const double best = optimize(workflow.graph, workflow.output, optimizer, 400,
                                 [](double value) {
                                     Feedback feedback;
                                     feedback.score = value;
                                     feedback.text = "the output should be smaller";
                                     return feedback;
                                 });

    TYST_EXPECT_LT(best, 1e-3);
    TYST_EXPECT_LT(optimizer.best_score(), 1e-3);
    TYST_EXPECT_EQ(optimizer.iterations(), 400u);
    TYST_EXPECT_EQ(optimizer.tracked_parameters().size(), 2u);
    // Never touched: the trace proved it could not affect the output.
    TYST_EXPECT_NEAR(workflow.graph.parameter_value(workflow.unused), 42.0, 1e-12);
}

TYST_TEST(TraceOptoTest, OptoPrimeKeepsBoundedMemoryOfAttempts) {
    QuadraticWorkflow workflow;
    OptoPrime::Config config;
    config.memory_size = 8;
    OptoPrime optimizer(config);

    optimize(workflow.graph, workflow.output, optimizer, 50, [](double value) {
        Feedback feedback;
        feedback.score = value;
        return feedback;
    });

    TYST_EXPECT_EQ(optimizer.memory().size(), 8u);
    TYST_EXPECT_EQ(optimizer.name(), std::string("OptoPrime"));
}

TYST_TEST(TraceOptoTest, NonDifferentiableOperationsAreFine) {
    TraceGraph graph;
    const NodeId threshold = graph.parameter("threshold", 0.0);
    // A step function has no useful derivative anywhere, which is exactly the
    // kind of workflow OPTO is meant to handle.
    const NodeId output = graph.apply("step", {threshold}, [](const std::vector<double>& v) {
        return v[0] > 1.0 ? 0.0 : 1.0;
    });

    TYST_EXPECT_NEAR(graph.forward(output), 1.0, 1e-12);
    graph.set_parameter(threshold, 2.0);
    TYST_EXPECT_NEAR(graph.forward(output), 0.0, 1e-12);
    TYST_EXPECT_EQ(graph.trace(output).parameters.size(), 1u);
}

TYST_TEST(TraceOptoTest, RejectsMisuse) {
    TraceGraph graph;
    const NodeId constant = graph.constant("c", 1.0);
    TYST_EXPECT_THROW(graph.set_parameter(constant, 2.0), std::invalid_argument);
    TYST_EXPECT_THROW(graph.value(999), std::out_of_range);
}

// ============================================================================
// LLF / HELiX (arXiv:2506.10341)
// ============================================================================

TYST_TEST(LlfTest, RichLanguageFeedbackCollapsesTheEluderDimension) {
    const size_t actions = 6;
    const size_t reward_only = transfer_eluder_dimension(make_reward_only_problem(actions, 5));
    const size_t rich = transfer_eluder_dimension(make_rich_feedback_problem(actions, 5));

    // Reward-only feedback forces the learner to rule out actions one by one.
    TYST_EXPECT_EQ(reward_only, actions - 1);
    // A sentence that names a better action settles everything in one shot.
    TYST_EXPECT_EQ(rich, 1u);
    TYST_EXPECT_LT(rich, reward_only);
}

TYST_TEST(LlfTest, HelixIdentifiesTheTruthFromOneRichHint) {
    Helix learner(make_rich_feedback_problem(6, 3));

    TYST_EXPECT_EQ(learner.version_space().size(), 6u);
    learner.interact();
    TYST_EXPECT_TRUE(learner.identified());

    const size_t second = learner.interact();
    TYST_EXPECT_EQ(second, 3u);
    TYST_EXPECT_EQ(learner.mistakes(), 1u);
    TYST_EXPECT_NEAR(learner.cumulative_regret(), 1.0, 1e-12);
}

TYST_TEST(LlfTest, HelixRegretMatchesTheTransferEluderDimension) {
    const size_t actions = 6;
    const LlfProblem problem = make_reward_only_problem(actions, actions - 1);
    const size_t dimension = transfer_eluder_dimension(problem);

    Helix learner(problem);
    for (size_t round = 0; round < 20; ++round) {
        learner.interact();
    }

    // Optimism plus elimination costs one mistake per independent action.
    TYST_EXPECT_EQ(learner.mistakes(), dimension);
    TYST_EXPECT_NEAR(learner.cumulative_regret(), static_cast<double>(dimension), 1e-12);
    TYST_EXPECT_TRUE(learner.identified());
}

TYST_TEST(LlfTest, VersionSpaceShrinksMonotonically) {
    Helix learner(make_reward_only_problem(5, 4));

    size_t previous = learner.version_space().size();
    for (size_t round = 0; round < 5; ++round) {
        learner.interact();
        const size_t current = learner.version_space().size();
        TYST_EXPECT_LE(current, previous);
        previous = current;
    }
    TYST_EXPECT_TRUE(learner.identified());
}

TYST_TEST(LlfTest, MemorylessPromptingNeverConverges) {
    const size_t actions = 6;
    const size_t rounds = 20;
    const LlfProblem problem = make_reward_only_problem(actions, actions - 1);

    Helix learner(problem);
    MemorylessPrompting baseline(problem);
    for (size_t round = 0; round < rounds; ++round) {
        learner.interact();
        baseline.interact();
    }

    // Forgetting all but the last observation makes the learner oscillate
    // between two plausible actions and accumulate linear regret.
    TYST_EXPECT_EQ(baseline.mistakes(), rounds);
    TYST_EXPECT_LT(learner.mistakes(), baseline.mistakes());
    TYST_EXPECT_LT(learner.cumulative_regret(), baseline.cumulative_regret());
}

TYST_TEST(LlfTest, ProblemAccessorsAndValidation) {
    const LlfProblem problem = make_rich_feedback_problem(4, 2);
    TYST_EXPECT_NEAR(problem.optimal_reward(), 1.0, 1e-12);
    TYST_EXPECT_NEAR(problem.true_reward(2), 1.0, 1e-12);
    TYST_EXPECT_NEAR(problem.true_reward(0), 0.0, 1e-12);
    TYST_EXPECT_NE(problem.true_feedback(0).find("try action 2"), std::string::npos);

    LlfProblem broken = problem;
    broken.truth = 99;
    TYST_EXPECT_THROW((void) Helix(broken), std::out_of_range);
}
