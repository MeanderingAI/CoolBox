#include "agent_runtime.h"
#include "distributed_agent_runtime.h"
#include "tyst_framework.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace ml::deep_learning::agent_runtime;

TEST(ToolRegistryTest, EmitsOpenAiSchemaAndInvokesTool) {
    ToolRegistry registry;
    ToolDefinition definition;
    definition.name = "get_weather";
    definition.description = "Get weather for a city";
    definition.parameters_json_schema =
        R"({"type":"object","properties":{"city":{"type":"string"}},"required":["city"],"additionalProperties":false})";
    registry.register_tool(
        definition,
        [](const ToolCall& call, ExecutionContext&) {
            return ToolResult{
                call.id, call.name, "sunny", false};
        });

    const std::string schema = registry.openai_tools_json();
    EXPECT_NE(schema.find(R"("type":"function")"), std::string::npos);
    EXPECT_NE(schema.find(R"("name":"get_weather")"), std::string::npos);
    EXPECT_NE(schema.find(R"("strict":true)"), std::string::npos);

    ExecutionContext context;
    const ToolResult result = registry.invoke(
        ToolCall{"call-1", "get_weather", R"({"city":"Seattle"})"},
        context);
    EXPECT_EQ(result.content, "sunny");
}

TEST(MemoryTest, SeparatesKindsAndRanksRelevantRecords) {
    InMemoryMemoryStore memory;
    MemoryRecord episodic;
    episodic.kind = MemoryKind::episodic;
    episodic.content = "The user prefers Vulkan acceleration";
    episodic.importance = 0.8;
    memory.remember(episodic);

    MemoryRecord procedural;
    procedural.kind = MemoryKind::procedural;
    procedural.content = "Build the project with CMake";
    memory.remember(procedural);

    MemoryQuery query;
    query.text = "Vulkan GPU";
    query.kinds = {MemoryKind::episodic};
    const auto matches = memory.recall(query);
    EXPECT_EQ(matches.size(), 1u);
    EXPECT_NE(matches.front().record.content.find("Vulkan"), std::string::npos);
}

TEST(AgentRuntimeTest, ExecutesDependenciesRetriesAndEvaluates) {
    auto tools = std::make_shared<ToolRegistry>();
    std::size_t attempts = 0;
    ToolDefinition fetch;
    fetch.name = "fetch";
    fetch.description = "Fetch evidence";
    tools->register_tool(
        fetch,
        [&attempts](const ToolCall& call, ExecutionContext&) {
            ++attempts;
            if (attempts == 1) {
                throw std::runtime_error("temporary failure");
            }
            return ToolResult{call.id, call.name, "evidence", false};
        });
    ToolDefinition answer;
    answer.name = "answer";
    answer.description = "Create the answer";
    tools->register_tool(
        answer,
        [](const ToolCall& call, ExecutionContext&) {
            return ToolResult{call.id, call.name, "final answer", false};
        });

    auto events = std::make_shared<InMemoryEventSink>();
    auto evaluations = std::make_shared<InMemoryEvaluationSink>();
    AgentRuntime runtime(
        tools,
        std::make_shared<InMemoryMemoryStore>(),
        std::make_shared<InMemoryRunStateStore>(),
        events,
        evaluations);
    runtime.add_evaluator([](const AgentRunState& state) {
        EvaluationRecord evaluation;
        evaluation.evaluator = "success";
        evaluation.passed = state.status == RunStatus::succeeded;
        evaluation.score = evaluation.passed ? 1.0 : 0.0;
        return evaluation;
    });

    Plan plan;
    PlanStep first;
    first.id = "fetch";
    first.objective = "Fetch evidence";
    first.tool = "fetch";
    first.max_attempts = 2;
    plan.steps.push_back(first);
    PlanStep second;
    second.id = "answer";
    second.objective = "Answer";
    second.tool = "answer";
    second.dependencies = {"fetch"};
    plan.steps.push_back(second);

    const AgentRunState state = runtime.run("Answer the question", plan);
    EXPECT_EQ(state.status, RunStatus::succeeded);
    EXPECT_EQ(state.output, "final answer");
    EXPECT_EQ(state.steps.front().attempts, 2u);
    EXPECT_GT(events->events().size(), 4u);
    EXPECT_EQ(evaluations->evaluations().size(), 1u);
    EXPECT_TRUE(evaluations->evaluations().front().passed);
}

TEST(AgentRuntimeTest, GuardrailBlocksToolExecution) {
    auto tools = std::make_shared<ToolRegistry>();
    ToolDefinition definition;
    definition.name = "dangerous";
    definition.description = "A blocked tool";
    tools->register_tool(
        definition,
        [](const ToolCall&, ExecutionContext&) {
            throw std::runtime_error("must not execute");
            return ToolResult{};
        });
    AgentRuntime runtime(tools);
    runtime.add_guardrail(
        [](GuardrailPhase phase,
           const std::string&,
           const ExecutionContext&) {
            if (phase == GuardrailPhase::tool_call) {
                return GuardrailDecision{false, "tool denied"};
            }
            return GuardrailDecision{};
        });
    Plan plan;
    PlanStep step;
    step.id = "blocked";
    step.objective = "Do not run";
    step.tool = "dangerous";
    plan.steps.push_back(step);

    const AgentRunState state = runtime.run("blocked request", plan);
    EXPECT_EQ(state.status, RunStatus::blocked);
    EXPECT_EQ(state.error, "tool denied");
}

TEST(DistributedAgentRuntimeTest, ExecutesIndependentStepsInParallel) {
    const std::string cluster = "agent-runtime-parallel-test";
    auto coordinator_context =
        std::make_shared<distributed::DistributedContext>(
            3, 0, cluster);
    auto first_context =
        std::make_shared<distributed::DistributedContext>(
            3, 1, cluster);
    auto second_context =
        std::make_shared<distributed::DistributedContext>(
            3, 2, cluster);
    auto tools = std::make_shared<ToolRegistry>();
    std::atomic<int> active{0};
    std::atomic<int> maximum_active{0};
    ToolDefinition definition;
    definition.name = "parallel";
    definition.description = "Run concurrently";
    tools->register_tool(
        definition,
        [&](const ToolCall& call, ExecutionContext&) {
            const int current = ++active;
            int observed = maximum_active.load();
            while (current > observed &&
                   !maximum_active.compare_exchange_weak(
                       observed, current)) {
            }
            const auto deadline =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(500);
            while (maximum_active.load() < 2 &&
                   std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
            }
            --active;
            return ToolResult{
                call.id, call.name, "complete", false};
        });

    AgentWorker first(first_context, tools);
    AgentWorker second(second_context, tools);
    first.start();
    second.start();

    DistributedAgentRuntime runtime(coordinator_context, tools);
    Plan plan;
    for (const std::string& id : {"first", "second"}) {
        PlanStep step;
        step.id = id;
        step.objective = id;
        step.tool = "parallel";
        plan.steps.push_back(step);
    }
    const AgentRunState state = runtime.run("parallel work", plan);
    first.stop();
    second.stop();
    first.join();
    second.join();

    EXPECT_EQ(state.status, RunStatus::succeeded);
    EXPECT_EQ(maximum_active.load(), 2);
}

int main(int argc, char** argv) {
    tyst::framework::init(&argc, argv);
    return tyst::framework::run_all_tests();
}
