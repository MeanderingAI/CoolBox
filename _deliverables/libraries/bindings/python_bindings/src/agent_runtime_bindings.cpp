#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "agent_runtime.h"
#include "distributed_agent_runtime.h"

namespace py = pybind11;

namespace {

using namespace ml::deep_learning::agent_runtime;

class PyMemoryStore : public MemoryStore {
public:
    using MemoryStore::MemoryStore;

    std::string remember(MemoryRecord record) override {
        PYBIND11_OVERRIDE_PURE(
            std::string, MemoryStore, remember, std::move(record));
    }

    std::vector<MemoryMatch> recall(
        const MemoryQuery& query) const override {
        PYBIND11_OVERRIDE_PURE(
            std::vector<MemoryMatch>, MemoryStore, recall, query);
    }

    bool forget(const std::string& id) override {
        PYBIND11_OVERRIDE_PURE(bool, MemoryStore, forget, id);
    }

    void clear(const std::string& name_space) override {
        PYBIND11_OVERRIDE_PURE(
            void, MemoryStore, clear, name_space);
    }
};

class PyEventSink : public EventSink {
public:
    using EventSink::EventSink;

    void emit(const AgentEvent& event) override {
        PYBIND11_OVERRIDE_PURE(void, EventSink, emit, event);
    }
};

class PyEvaluationSink : public EvaluationSink {
public:
    using EvaluationSink::EvaluationSink;

    void record(const EvaluationRecord& evaluation) override {
        PYBIND11_OVERRIDE_PURE(
            void, EvaluationSink, record, evaluation);
    }
};

class PyRunStateStore : public RunStateStore {
public:
    using RunStateStore::RunStateStore;

    void save(const AgentRunState& state) override {
        PYBIND11_OVERRIDE_PURE(void, RunStateStore, save, state);
    }

    std::optional<AgentRunState> load(
        const std::string& run_id) const override {
        PYBIND11_OVERRIDE_PURE(
            std::optional<AgentRunState>,
            RunStateStore,
            load,
            run_id);
    }
};

class PyCredentialProvider : public CredentialProvider {
public:
    using CredentialProvider::CredentialProvider;

    std::optional<std::string> resolve(
        const std::string& name) const override {
        PYBIND11_OVERRIDE_PURE(
            std::optional<std::string>,
            CredentialProvider,
            resolve,
            name);
    }
};

py::dict openai_definition(const ToolDefinition& definition) {
    py::module_ json = py::module_::import("json");
    py::dict function;
    function["name"] = definition.name;
    function["description"] = definition.description;
    function["parameters"] =
        json.attr("loads")(definition.parameters_json_schema);
    function["strict"] = definition.strict;

    py::dict tool;
    tool["type"] = "function";
    tool["function"] = std::move(function);
    return tool;
}

ToolCall parse_chat_tool_call(const py::dict& value) {
    if (!value.contains("id") || !value.contains("function")) {
        throw py::value_error(
            "Chat Completions tool call requires id and function");
    }
    const py::dict function = value["function"].cast<py::dict>();
    if (!function.contains("name") ||
        !function.contains("arguments")) {
        throw py::value_error(
            "Chat Completions function requires name and arguments");
    }
    ToolCall call;
    call.id = py::str(value["id"]);
    call.name = py::str(function["name"]);
    call.arguments_json = py::str(function["arguments"]);
    return call;
}

ToolCall parse_response_tool_call(const py::dict& value) {
    if (!value.contains("name") || !value.contains("arguments")) {
        throw py::value_error(
            "Responses API function call requires name and arguments");
    }
    ToolCall call;
    if (value.contains("call_id")) {
        call.id = py::str(value["call_id"]);
    } else if (value.contains("id")) {
        call.id = py::str(value["id"]);
    } else {
        throw py::value_error(
            "Responses API function call requires call_id or id");
    }
    call.name = py::str(value["name"]);
    call.arguments_json = py::str(value["arguments"]);
    return call;
}

} // namespace

void bind_agent_runtime(py::module_& parent) {
    using namespace ml::deep_learning::agent_runtime;

    py::module_ module = parent.def_submodule(
        "agent_runtime",
        "Durable-agent contracts, OpenAI tools, memory, planning, and traces");

    py::enum_<MemoryKind>(module, "MemoryKind")
        .value("WORKING", MemoryKind::working)
        .value("EPISODIC", MemoryKind::episodic)
        .value("SEMANTIC", MemoryKind::semantic)
        .value("PROCEDURAL", MemoryKind::procedural);
    py::class_<MemoryRecord>(module, "MemoryRecord")
        .def(py::init<>())
        .def_readwrite("id", &MemoryRecord::id)
        .def_readwrite("namespace", &MemoryRecord::name_space)
        .def_readwrite("kind", &MemoryRecord::kind)
        .def_readwrite("content", &MemoryRecord::content)
        .def_readwrite("metadata", &MemoryRecord::metadata)
        .def_readwrite("importance", &MemoryRecord::importance)
        .def_readwrite("created_at_ms", &MemoryRecord::created_at_ms)
        .def_readwrite("expires_at_ms", &MemoryRecord::expires_at_ms);
    py::class_<MemoryQuery>(module, "MemoryQuery")
        .def(py::init<>())
        .def_readwrite("namespace", &MemoryQuery::name_space)
        .def_readwrite("kinds", &MemoryQuery::kinds)
        .def_readwrite("text", &MemoryQuery::text)
        .def_readwrite("limit", &MemoryQuery::limit)
        .def_readwrite("minimum_score", &MemoryQuery::minimum_score);
    py::class_<MemoryMatch>(module, "MemoryMatch")
        .def(py::init<>())
        .def_readwrite("record", &MemoryMatch::record)
        .def_readwrite("score", &MemoryMatch::score);
    py::class_<
        MemoryStore,
        PyMemoryStore,
        std::shared_ptr<MemoryStore>>(module, "MemoryStore")
        .def(py::init<>());
    py::class_<
        InMemoryMemoryStore,
        MemoryStore,
        std::shared_ptr<InMemoryMemoryStore>>(
        module, "InMemoryMemoryStore")
        .def(py::init<>())
        .def("remember", &InMemoryMemoryStore::remember)
        .def("recall", &InMemoryMemoryStore::recall)
        .def("forget", &InMemoryMemoryStore::forget)
        .def("clear", &InMemoryMemoryStore::clear)
        .def("__len__", &InMemoryMemoryStore::size);

    py::class_<ExecutionContext>(module, "ExecutionContext")
        .def(py::init<>())
        .def_readwrite("run_id", &ExecutionContext::run_id)
        .def_readwrite("agent_id", &ExecutionContext::agent_id)
        .def_readwrite("metadata", &ExecutionContext::metadata)
        .def_readwrite("credentials", &ExecutionContext::credentials)
        .def_readwrite("memory", &ExecutionContext::memory);
    py::class_<ToolDefinition>(module, "ToolDefinition")
        .def(py::init<>())
        .def_readwrite("name", &ToolDefinition::name)
        .def_readwrite("description", &ToolDefinition::description)
        .def_readwrite(
            "parameters_json_schema",
            &ToolDefinition::parameters_json_schema)
        .def_readwrite("strict", &ToolDefinition::strict)
        .def_readwrite(
            "required_credentials",
            &ToolDefinition::required_credentials)
        .def_readwrite(
            "sandbox_required",
            &ToolDefinition::sandbox_required);
    py::class_<ToolCall>(module, "ToolCall")
        .def(py::init<>())
        .def_readwrite("id", &ToolCall::id)
        .def_readwrite("name", &ToolCall::name)
        .def_readwrite("arguments_json", &ToolCall::arguments_json);
    py::class_<ToolResult>(module, "ToolResult")
        .def(py::init<>())
        .def_readwrite("call_id", &ToolResult::call_id)
        .def_readwrite("name", &ToolResult::name)
        .def_readwrite("content", &ToolResult::content)
        .def_readwrite("is_error", &ToolResult::is_error);
    py::class_<
        CredentialProvider,
        PyCredentialProvider,
        std::shared_ptr<CredentialProvider>>(
        module, "CredentialProvider")
        .def(py::init<>());
    py::class_<
        EnvironmentCredentialProvider,
        CredentialProvider,
        std::shared_ptr<EnvironmentCredentialProvider>>(
        module, "EnvironmentCredentialProvider")
        .def(py::init<std::vector<std::string>>(),
             py::arg("allowed_names"));
    py::class_<ToolPolicyDecision>(module, "ToolPolicyDecision")
        .def(py::init<>())
        .def_readwrite("allowed", &ToolPolicyDecision::allowed)
        .def_readwrite("reason", &ToolPolicyDecision::reason);
    py::class_<ToolRegistry, std::shared_ptr<ToolRegistry>>(
        module, "ToolRegistry")
        .def(py::init<>())
        .def("register_tool", &ToolRegistry::register_tool)
        .def(
            "register_openai_tool",
            [](ToolRegistry& registry,
               const std::string& name,
               const std::string& description,
               const py::dict& parameters,
               ToolHandler handler,
               bool strict,
               const std::vector<std::string>& required_credentials,
               bool sandbox_required) {
                py::module_ json = py::module_::import("json");
                ToolDefinition definition;
                definition.name = name;
                definition.description = description;
                definition.parameters_json_schema =
                    py::str(json.attr("dumps")(
                        parameters,
                        py::arg("separators") =
                            py::make_tuple(",", ":")));
                definition.strict = strict;
                definition.required_credentials =
                    required_credentials;
                definition.sandbox_required =
                    sandbox_required;
                registry.register_tool(
                    std::move(definition), std::move(handler));
            },
            py::arg("name"),
            py::arg("description"),
            py::arg("parameters"),
            py::arg("handler"),
            py::arg("strict") = true,
            py::arg("required_credentials") =
                std::vector<std::string>(),
            py::arg("sandbox_required") = false)
        .def("remove_tool", &ToolRegistry::remove_tool)
        .def("__contains__", &ToolRegistry::contains)
        .def(
            "set_credential_provider",
            &ToolRegistry::set_credential_provider)
        .def(
            "set_execution_policy",
            &ToolRegistry::set_execution_policy)
        .def_property_readonly(
            "definitions", &ToolRegistry::definitions)
        .def("invoke", &ToolRegistry::invoke)
        .def(
            "openai_tools",
            [](const ToolRegistry& registry) {
                py::list result;
                for (const ToolDefinition& definition :
                     registry.definitions()) {
                    result.append(openai_definition(definition));
                }
                return result;
            })
        .def("openai_tools_json", &ToolRegistry::openai_tools_json)
        .def(
            "invoke_openai_chat_tool_call",
            [](const ToolRegistry& registry,
               const py::dict& tool_call,
               ExecutionContext& context) {
                const ToolResult result = registry.invoke(
                    parse_chat_tool_call(tool_call), context);
                py::dict message;
                message["role"] = "tool";
                message["tool_call_id"] = result.call_id;
                message["content"] = result.content;
                return message;
            },
            py::arg("tool_call"),
            py::arg("context"))
        .def(
            "invoke_openai_response_tool_call",
            [](const ToolRegistry& registry,
               const py::dict& tool_call,
               ExecutionContext& context) {
                const ToolResult result = registry.invoke(
                    parse_response_tool_call(tool_call), context);
                py::dict output;
                output["type"] = "function_call_output";
                output["call_id"] = result.call_id;
                output["output"] = result.content;
                return output;
            },
            py::arg("tool_call"),
            py::arg("context"));

    py::enum_<PlanStepStatus>(module, "PlanStepStatus")
        .value("PENDING", PlanStepStatus::pending)
        .value("RUNNING", PlanStepStatus::running)
        .value("COMPLETED", PlanStepStatus::completed)
        .value("FAILED", PlanStepStatus::failed)
        .value("BLOCKED", PlanStepStatus::blocked);
    py::class_<PlanStep>(module, "PlanStep")
        .def(py::init<>())
        .def_readwrite("id", &PlanStep::id)
        .def_readwrite("objective", &PlanStep::objective)
        .def_readwrite("tool", &PlanStep::tool)
        .def_readwrite("arguments_json", &PlanStep::arguments_json)
        .def_readwrite("dependencies", &PlanStep::dependencies)
        .def_readwrite("max_attempts", &PlanStep::max_attempts)
        .def_readwrite(
            "requires_approval", &PlanStep::requires_approval)
        .def_readwrite("agent_id", &PlanStep::agent_id)
        .def_readwrite("worker_rank", &PlanStep::worker_rank);
    py::class_<Plan>(module, "Plan")
        .def(py::init<>())
        .def_readwrite("steps", &Plan::steps)
        .def("validate", &Plan::validate);
    py::enum_<RunStatus>(module, "RunStatus")
        .value("PENDING", RunStatus::pending)
        .value("RUNNING", RunStatus::running)
        .value("SUCCEEDED", RunStatus::succeeded)
        .value("FAILED", RunStatus::failed)
        .value("BLOCKED", RunStatus::blocked);
    py::class_<StepExecution>(module, "StepExecution")
        .def(py::init<>())
        .def_readwrite("step", &StepExecution::step)
        .def_readwrite("status", &StepExecution::status)
        .def_readwrite("attempts", &StepExecution::attempts)
        .def_readwrite("result", &StepExecution::result)
        .def_readwrite("error", &StepExecution::error);
    py::class_<AgentRunState>(module, "AgentRunState")
        .def(py::init<>())
        .def_readwrite("run_id", &AgentRunState::run_id)
        .def_readwrite("goal", &AgentRunState::goal)
        .def_readwrite("status", &AgentRunState::status)
        .def_readwrite("steps", &AgentRunState::steps)
        .def_readwrite("output", &AgentRunState::output)
        .def_readwrite("error", &AgentRunState::error)
        .def_readwrite(
            "next_event_sequence",
            &AgentRunState::next_event_sequence);

    py::class_<AgentEvent>(module, "AgentEvent")
        .def(py::init<>())
        .def_readwrite("sequence", &AgentEvent::sequence)
        .def_readwrite("timestamp_ms", &AgentEvent::timestamp_ms)
        .def_readwrite("run_id", &AgentEvent::run_id)
        .def_readwrite("type", &AgentEvent::type)
        .def_readwrite("step_id", &AgentEvent::step_id)
        .def_readwrite("message", &AgentEvent::message)
        .def_readwrite("attributes", &AgentEvent::attributes);
    py::class_<
        EventSink,
        PyEventSink,
        std::shared_ptr<EventSink>>(module, "EventSink")
        .def(py::init<>());
    py::class_<
        InMemoryEventSink,
        EventSink,
        std::shared_ptr<InMemoryEventSink>>(
        module, "InMemoryEventSink")
        .def(py::init<>())
        .def_property_readonly("events", &InMemoryEventSink::events)
        .def("clear", &InMemoryEventSink::clear);
    py::class_<EvaluationRecord>(module, "EvaluationRecord")
        .def(py::init<>())
        .def_readwrite("run_id", &EvaluationRecord::run_id)
        .def_readwrite("evaluator", &EvaluationRecord::evaluator)
        .def_readwrite("score", &EvaluationRecord::score)
        .def_readwrite("passed", &EvaluationRecord::passed)
        .def_readwrite("feedback", &EvaluationRecord::feedback)
        .def_readwrite("metrics", &EvaluationRecord::metrics);
    py::class_<
        EvaluationSink,
        PyEvaluationSink,
        std::shared_ptr<EvaluationSink>>(module, "EvaluationSink")
        .def(py::init<>());
    py::class_<
        InMemoryEvaluationSink,
        EvaluationSink,
        std::shared_ptr<InMemoryEvaluationSink>>(
        module, "InMemoryEvaluationSink")
        .def(py::init<>())
        .def_property_readonly(
            "evaluations",
            &InMemoryEvaluationSink::evaluations);
    py::class_<
        RunStateStore,
        PyRunStateStore,
        std::shared_ptr<RunStateStore>>(module, "RunStateStore")
        .def(py::init<>());
    py::class_<
        InMemoryRunStateStore,
        RunStateStore,
        std::shared_ptr<InMemoryRunStateStore>>(
        module, "InMemoryRunStateStore")
        .def(py::init<>())
        .def("save", &InMemoryRunStateStore::save)
        .def("load", &InMemoryRunStateStore::load);

    py::enum_<GuardrailPhase>(module, "GuardrailPhase")
        .value("INPUT", GuardrailPhase::input)
        .value("TOOL_CALL", GuardrailPhase::tool_call)
        .value("TOOL_RESULT", GuardrailPhase::tool_result)
        .value("OUTPUT", GuardrailPhase::output);
    py::class_<GuardrailDecision>(module, "GuardrailDecision")
        .def(py::init<>())
        .def_readwrite("allowed", &GuardrailDecision::allowed)
        .def_readwrite("reason", &GuardrailDecision::reason);
    py::class_<ApprovalRequest>(module, "ApprovalRequest")
        .def_readonly("run_id", &ApprovalRequest::run_id)
        .def_readonly("goal", &ApprovalRequest::goal)
        .def_readonly("step", &ApprovalRequest::step);
    py::class_<ApprovalDecision>(module, "ApprovalDecision")
        .def(py::init<>())
        .def_readwrite("approved", &ApprovalDecision::approved)
        .def_readwrite("reason", &ApprovalDecision::reason);
    py::class_<AgentRuntimeConfig>(module, "AgentRuntimeConfig")
        .def(py::init<>())
        .def_readwrite("max_steps", &AgentRuntimeConfig::max_steps)
        .def_readwrite("stop_on_error", &AgentRuntimeConfig::stop_on_error)
        .def_readwrite("agent_id", &AgentRuntimeConfig::agent_id)
        .def_readwrite(
            "memory_namespace",
            &AgentRuntimeConfig::memory_name_space);
    py::class_<AgentRuntime>(module, "AgentRuntime")
        .def(
            py::init([](
                std::shared_ptr<ToolRegistry> tools,
                std::shared_ptr<MemoryStore> memory,
                std::shared_ptr<RunStateStore> states,
                std::shared_ptr<EventSink> events,
                std::shared_ptr<EvaluationSink> evaluations,
                AgentRuntimeConfig config) {
                if (!memory) {
                    memory = std::make_shared<InMemoryMemoryStore>();
                }
                if (!states) {
                    states = std::make_shared<InMemoryRunStateStore>();
                }
                if (!events) {
                    events = std::make_shared<InMemoryEventSink>();
                }
                if (!evaluations) {
                    evaluations =
                        std::make_shared<InMemoryEvaluationSink>();
                }
                return std::make_unique<AgentRuntime>(
                    std::move(tools),
                    std::move(memory),
                    std::move(states),
                    std::move(events),
                    std::move(evaluations),
                    std::move(config));
            }),
            py::arg("tools"),
            py::arg("memory") = nullptr,
            py::arg("states") = nullptr,
            py::arg("events") = nullptr,
            py::arg("evaluations") = nullptr,
            py::arg("config") = AgentRuntimeConfig())
        .def("add_guardrail", &AgentRuntime::add_guardrail)
        .def("add_evaluator", &AgentRuntime::add_evaluator)
        .def(
            "set_approval_handler",
            &AgentRuntime::set_approval_handler)
        .def("run", &AgentRuntime::run)
        .def("run_with_planner", &AgentRuntime::run_with_planner)
        .def("resume", &AgentRuntime::resume);

    py::class_<DistributedRuntimeConfig>(
        module, "DistributedRuntimeConfig")
        .def(py::init<>())
        .def_readwrite(
            "max_steps", &DistributedRuntimeConfig::max_steps)
        .def_readwrite(
            "task_timeout_ms",
            &DistributedRuntimeConfig::task_timeout_ms)
        .def_readwrite(
            "receive_poll_ms",
            &DistributedRuntimeConfig::receive_poll_ms)
        .def_readwrite(
            "stop_on_error",
            &DistributedRuntimeConfig::stop_on_error)
        .def_readwrite(
            "agent_id", &DistributedRuntimeConfig::agent_id);
    py::class_<AgentWorker>(module, "AgentWorker")
        .def(py::init<
             std::shared_ptr<distributed::DistributedContext>,
             std::shared_ptr<ToolRegistry>>())
        .def("start", &AgentWorker::start)
        .def(
            "stop",
            &AgentWorker::stop)
        .def(
            "join",
            &AgentWorker::join,
            py::call_guard<py::gil_scoped_release>())
        .def_property_readonly("running", &AgentWorker::running)
        .def_property_readonly("rank", &AgentWorker::rank);
    py::class_<DistributedAgentRuntime>(
        module, "DistributedAgentRuntime")
        .def(
            py::init([](
                std::shared_ptr<distributed::DistributedContext> coordinator,
                std::shared_ptr<ToolRegistry> tools,
                std::shared_ptr<MemoryStore> memory,
                std::shared_ptr<RunStateStore> states,
                std::shared_ptr<EventSink> events,
                std::shared_ptr<EvaluationSink> evaluations,
                DistributedRuntimeConfig config) {
                if (!memory) {
                    memory = std::make_shared<InMemoryMemoryStore>();
                }
                if (!states) {
                    states = std::make_shared<InMemoryRunStateStore>();
                }
                if (!events) {
                    events = std::make_shared<InMemoryEventSink>();
                }
                if (!evaluations) {
                    evaluations =
                        std::make_shared<InMemoryEvaluationSink>();
                }
                return std::make_unique<DistributedAgentRuntime>(
                    std::move(coordinator),
                    std::move(tools),
                    std::move(memory),
                    std::move(states),
                    std::move(events),
                    std::move(evaluations),
                    std::move(config));
            }),
            py::arg("coordinator"),
            py::arg("tools"),
            py::arg("memory") = nullptr,
            py::arg("states") = nullptr,
            py::arg("events") = nullptr,
            py::arg("evaluations") = nullptr,
            py::arg("config") = DistributedRuntimeConfig())
        .def(
            "add_guardrail",
            &DistributedAgentRuntime::add_guardrail)
        .def(
            "add_evaluator",
            &DistributedAgentRuntime::add_evaluator)
        .def(
            "set_approval_handler",
            &DistributedAgentRuntime::set_approval_handler)
        .def(
            "run",
            &DistributedAgentRuntime::run,
            py::call_guard<py::gil_scoped_release>())
        .def(
            "run_with_planner",
            &DistributedAgentRuntime::run_with_planner,
            py::call_guard<py::gil_scoped_release>())
        .def(
            "resume",
            &DistributedAgentRuntime::resume,
            py::call_guard<py::gil_scoped_release>())
        .def("cancel", &DistributedAgentRuntime::cancel);
}
