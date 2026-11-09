#include "agent_runtime.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace ml {
namespace deep_learning {
namespace agent_runtime {
namespace {

std::uint64_t now_ms() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string json_escape(const std::string& value) {
    std::ostringstream stream;
    for (unsigned char character : value) {
        switch (character) {
        case '"':
            stream << "\\\"";
            break;
        case '\\':
            stream << "\\\\";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (character < 0x20) {
                const char* digits = "0123456789abcdef";
                stream << "\\u00"
                       << digits[(character >> 4) & 0x0f]
                       << digits[character & 0x0f];
            } else {
                stream << static_cast<char>(character);
            }
        }
    }
    return stream.str();
}

std::unordered_set<std::string> terms(const std::string& text) {
    std::unordered_set<std::string> result;
    std::string term;
    for (unsigned char character : text) {
        if (std::isalnum(character)) {
            term.push_back(
                static_cast<char>(std::tolower(character)));
        } else if (!term.empty()) {
            result.insert(term);
            term.clear();
        }
    }
    if (!term.empty()) {
        result.insert(term);
    }
    return result;
}

bool includes_kind(
    const std::vector<MemoryKind>& kinds, MemoryKind kind) {
    return kinds.empty() ||
           std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

double memory_score(
    const MemoryRecord& record,
    const std::unordered_set<std::string>& query_terms) {
    if (query_terms.empty()) {
        return record.importance;
    }
    const auto record_terms = terms(record.content);
    std::size_t overlap = 0;
    for (const auto& term : query_terms) {
        overlap += record_terms.count(term);
    }
    const double relevance =
        static_cast<double>(overlap) /
        static_cast<double>(query_terms.size());
    return 0.8 * relevance + 0.2 * record.importance;
}

void validate_tool_name(const std::string& name) {
    if (name.empty() || name.size() > 64) {
        throw std::invalid_argument(
            "tool name must contain between 1 and 64 characters");
    }
    for (unsigned char character : name) {
        if (!(std::isalnum(character) || character == '_' ||
              character == '-')) {
            throw std::invalid_argument(
                "tool name may contain only letters, digits, '_' and '-'");
        }
    }
}

bool completed(const StepExecution& step) {
    return step.status == PlanStepStatus::completed;
}

} // namespace

std::string InMemoryMemoryStore::remember(MemoryRecord record) {
    if (record.name_space.empty()) {
        throw std::invalid_argument("memory namespace cannot be empty");
    }
    if (record.content.empty()) {
        throw std::invalid_argument("memory content cannot be empty");
    }
    if (!std::isfinite(record.importance) ||
        record.importance < 0.0 || record.importance > 1.0) {
        throw std::invalid_argument(
            "memory importance must be finite and between 0 and 1");
    }
    if (record.id.empty()) {
        record.id = "memory-" + std::to_string(next_id_++);
    } else if (records_.count(record.id) != 0) {
        throw std::invalid_argument(
            "memory id already exists: " + record.id);
    }
    if (record.created_at_ms == 0) {
        record.created_at_ms = now_ms();
    }
    const std::string id = record.id;
    records_.emplace(id, std::move(record));
    return id;
}

std::vector<MemoryMatch> InMemoryMemoryStore::recall(
    const MemoryQuery& query) const {
    if (query.limit == 0) {
        return {};
    }
    const auto query_terms = terms(query.text);
    const auto current_time = now_ms();
    std::vector<MemoryMatch> matches;
    for (const auto& item : records_) {
        const MemoryRecord& record = item.second;
        if (record.name_space != query.name_space ||
            !includes_kind(query.kinds, record.kind) ||
            (record.expires_at_ms != 0 &&
             record.expires_at_ms <= current_time)) {
            continue;
        }
        const double score = memory_score(record, query_terms);
        if (score >= query.minimum_score) {
            matches.push_back({record, score});
        }
    }
    std::sort(
        matches.begin(),
        matches.end(),
        [](const MemoryMatch& left, const MemoryMatch& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            if (left.record.created_at_ms != right.record.created_at_ms) {
                return left.record.created_at_ms >
                       right.record.created_at_ms;
            }
            return left.record.id < right.record.id;
        });
    if (matches.size() > query.limit) {
        matches.resize(query.limit);
    }
    return matches;
}

bool InMemoryMemoryStore::forget(const std::string& id) {
    return records_.erase(id) != 0;
}

void InMemoryMemoryStore::clear(const std::string& name_space) {
    for (auto iterator = records_.begin();
         iterator != records_.end();) {
        if (iterator->second.name_space == name_space) {
            iterator = records_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void ToolRegistry::register_tool(
    ToolDefinition definition, ToolHandler handler) {
    validate_tool_name(definition.name);
    if (definition.description.empty()) {
        throw std::invalid_argument(
            "tool description cannot be empty");
    }
    if (definition.parameters_json_schema.empty() ||
        definition.parameters_json_schema.front() != '{' ||
        definition.parameters_json_schema.back() != '}') {
        throw std::invalid_argument(
            "tool parameters_json_schema must be a JSON object");
    }
    if (!handler) {
        throw std::invalid_argument("tool handler cannot be empty");
    }
    if (tools_.count(definition.name) != 0) {
        throw std::invalid_argument(
            "tool already registered: " + definition.name);
    }
    const std::string name = definition.name;
    tools_.emplace(
        name, RegisteredTool{std::move(definition), std::move(handler)});
}

EnvironmentCredentialProvider::EnvironmentCredentialProvider(
    std::vector<std::string> allowed_names)
    : allowed_names_(std::move(allowed_names)) {
    for (const std::string& name : allowed_names_) {
        if (name.empty()) {
            throw std::invalid_argument(
                "credential names cannot be empty");
        }
    }
}

std::optional<std::string> EnvironmentCredentialProvider::resolve(
    const std::string& name) const {
    if (std::find(
            allowed_names_.begin(), allowed_names_.end(), name) ==
        allowed_names_.end()) {
        return std::nullopt;
    }
    const char* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

bool ToolRegistry::remove_tool(const std::string& name) {
    return tools_.erase(name) != 0;
}

bool ToolRegistry::contains(const std::string& name) const {
    return tools_.count(name) != 0;
}

std::vector<ToolDefinition> ToolRegistry::definitions() const {
    std::vector<ToolDefinition> result;
    result.reserve(tools_.size());
    for (const auto& item : tools_) {
        result.push_back(item.second.definition);
    }
    return result;
}

ToolResult ToolRegistry::invoke(
    const ToolCall& call, ExecutionContext& context) const {
    const auto iterator = tools_.find(call.name);
    if (iterator == tools_.end()) {
        throw std::invalid_argument(
            "unknown tool: " + call.name);
    }
    const ToolDefinition& definition = iterator->second.definition;
    for (const std::string& credential :
         definition.required_credentials) {
        if (!credential_provider_) {
            throw std::runtime_error(
                "tool requires a credential provider: " + call.name);
        }
        const auto value = credential_provider_->resolve(credential);
        if (!value.has_value()) {
            throw std::runtime_error(
                "required credential is unavailable: " + credential);
        }
        context.credentials[credential] = *value;
    }
    if (definition.sandbox_required && !execution_policy_) {
        throw std::runtime_error(
            "sandboxed tool requires an execution policy: " +
            call.name);
    }
    if (execution_policy_) {
        const ToolPolicyDecision decision =
            execution_policy_(definition, call, context);
        if (!decision.allowed) {
            throw std::runtime_error(
                decision.reason.empty()
                    ? "tool execution denied by policy"
                    : decision.reason);
        }
    }
    ToolResult result = iterator->second.handler(call, context);
    if (result.call_id.empty()) {
        result.call_id = call.id;
    }
    if (result.name.empty()) {
        result.name = call.name;
    }
    return result;
}

void ToolRegistry::set_credential_provider(
    std::shared_ptr<CredentialProvider> provider) {
    credential_provider_ = std::move(provider);
}

void ToolRegistry::set_execution_policy(
    ToolExecutionPolicy policy) {
    execution_policy_ = std::move(policy);
}

std::string ToolRegistry::openai_tools_json() const {
    std::ostringstream stream;
    stream << '[';
    bool first = true;
    for (const auto& item : tools_) {
        if (!first) {
            stream << ',';
        }
        first = false;
        const ToolDefinition& definition = item.second.definition;
        stream << R"({"type":"function","function":{"name":")"
               << json_escape(definition.name)
               << R"(","description":")"
               << json_escape(definition.description)
               << R"(","parameters":)"
               << definition.parameters_json_schema
               << R"(,"strict":)"
               << (definition.strict ? "true" : "false")
               << "}}";
    }
    stream << ']';
    return stream.str();
}

void Plan::validate() const {
    if (steps.empty()) {
        throw std::invalid_argument("plan must contain at least one step");
    }
    std::unordered_map<std::string, const PlanStep*> by_id;
    for (const PlanStep& step : steps) {
        if (step.id.empty()) {
            throw std::invalid_argument("plan step id cannot be empty");
        }
        if (step.tool.empty()) {
            throw std::invalid_argument(
                "plan step tool cannot be empty: " + step.id);
        }
        if (step.max_attempts == 0) {
            throw std::invalid_argument(
                "plan step max_attempts must be at least one: " +
                step.id);
        }
        if (!by_id.emplace(step.id, &step).second) {
            throw std::invalid_argument(
                "duplicate plan step id: " + step.id);
        }
    }
    for (const PlanStep& step : steps) {
        for (const std::string& dependency : step.dependencies) {
            if (dependency == step.id) {
                throw std::invalid_argument(
                    "plan step cannot depend on itself: " + step.id);
            }
            if (by_id.count(dependency) == 0) {
                throw std::invalid_argument(
                    "unknown dependency '" + dependency +
                    "' for step " + step.id);
            }
        }
    }

    std::unordered_map<std::string, int> colors;
    std::function<void(const std::string&)> visit =
        [&](const std::string& id) {
            if (colors[id] == 1) {
                throw std::invalid_argument(
                    "plan contains a dependency cycle at step " + id);
            }
            if (colors[id] == 2) {
                return;
            }
            colors[id] = 1;
            for (const std::string& dependency :
                 by_id.at(id)->dependencies) {
                visit(dependency);
            }
            colors[id] = 2;
        };
    for (const PlanStep& step : steps) {
        visit(step.id);
    }
}

void InMemoryEventSink::emit(const AgentEvent& event) {
    events_.push_back(event);
}

void InMemoryEvaluationSink::record(
    const EvaluationRecord& evaluation) {
    evaluations_.push_back(evaluation);
}

void InMemoryRunStateStore::save(const AgentRunState& state) {
    if (state.run_id.empty()) {
        throw std::invalid_argument("run id cannot be empty");
    }
    states_[state.run_id] = state;
}

std::optional<AgentRunState> InMemoryRunStateStore::load(
    const std::string& run_id) const {
    const auto iterator = states_.find(run_id);
    if (iterator == states_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

AgentRuntime::AgentRuntime(
    std::shared_ptr<ToolRegistry> tools,
    std::shared_ptr<MemoryStore> memory,
    std::shared_ptr<RunStateStore> states,
    std::shared_ptr<EventSink> events,
    std::shared_ptr<EvaluationSink> evaluations,
    AgentRuntimeConfig config)
    : tools_(std::move(tools)),
      memory_(std::move(memory)),
      states_(std::move(states)),
      events_(std::move(events)),
      evaluations_(std::move(evaluations)),
      config_(std::move(config)) {
    if (!tools_ || !memory_ || !states_ || !events_ || !evaluations_) {
        throw std::invalid_argument(
            "agent runtime dependencies cannot be null");
    }
    if (config_.max_steps == 0) {
        throw std::invalid_argument(
            "agent runtime max_steps must be at least one");
    }
}

void AgentRuntime::add_guardrail(Guardrail guardrail) {
    if (!guardrail) {
        throw std::invalid_argument("guardrail cannot be empty");
    }
    guardrails_.push_back(std::move(guardrail));
}

void AgentRuntime::add_evaluator(Evaluator evaluator) {
    if (!evaluator) {
        throw std::invalid_argument("evaluator cannot be empty");
    }
    evaluators_.push_back(std::move(evaluator));
}

void AgentRuntime::set_approval_handler(ApprovalHandler handler) {
    approval_handler_ = std::move(handler);
}

AgentRunState AgentRuntime::run(
    const std::string& goal, const Plan& plan) {
    if (goal.empty()) {
        throw std::invalid_argument("agent goal cannot be empty");
    }
    plan.validate();
    AgentRunState state;
    state.run_id = "run-" + std::to_string(next_run_id_++);
    state.goal = goal;
    for (const PlanStep& step : plan.steps) {
        StepExecution execution;
        execution.step = step;
        state.steps.push_back(std::move(execution));
    }
    return execute(std::move(state));
}

AgentRunState AgentRuntime::run_with_planner(
    const std::string& goal, const Planner& planner) {
    if (!planner) {
        throw std::invalid_argument("planner cannot be empty");
    }
    MemoryQuery query;
    query.name_space = config_.memory_name_space;
    query.text = goal;
    return run(goal, planner(goal, memory_->recall(query)));
}

AgentRunState AgentRuntime::resume(const std::string& run_id) {
    const auto state = states_->load(run_id);
    if (!state.has_value()) {
        throw std::invalid_argument("unknown run id: " + run_id);
    }
    if (state->status == RunStatus::succeeded ||
        state->status == RunStatus::blocked) {
        return *state;
    }
    AgentRunState resumed = *state;
    for (StepExecution& step : resumed.steps) {
        if (step.status == PlanStepStatus::running) {
            step.status = PlanStepStatus::pending;
        }
    }
    return execute(std::move(resumed));
}

GuardrailDecision AgentRuntime::check_guardrails(
    GuardrailPhase phase,
    const std::string& content,
    const ExecutionContext& context) const {
    for (const Guardrail& guardrail : guardrails_) {
        GuardrailDecision decision =
            guardrail(phase, content, context);
        if (!decision.allowed) {
            return decision;
        }
    }
    return {};
}

void AgentRuntime::emit(
    AgentRunState& state,
    const std::string& type,
    const std::string& message,
    const std::string& step_id) {
    AgentEvent event;
    event.sequence = state.next_event_sequence++;
    event.timestamp_ms = now_ms();
    event.run_id = state.run_id;
    event.type = type;
    event.step_id = step_id;
    event.message = message;
    events_->emit(event);
}

void AgentRuntime::persist(const AgentRunState& state) {
    states_->save(state);
}

void AgentRuntime::evaluate(const AgentRunState& state) {
    for (const Evaluator& evaluator : evaluators_) {
        EvaluationRecord record = evaluator(state);
        if (record.run_id.empty()) {
            record.run_id = state.run_id;
        }
        evaluations_->record(record);
    }
}

AgentRunState AgentRuntime::execute(AgentRunState state) {
    ExecutionContext context;
    context.run_id = state.run_id;
    context.agent_id = config_.agent_id;
    context.memory = memory_;

    const GuardrailDecision input_decision =
        check_guardrails(GuardrailPhase::input, state.goal, context);
    if (!input_decision.allowed) {
        state.status = RunStatus::blocked;
        state.error = input_decision.reason;
        emit(state, "run.blocked", state.error);
        persist(state);
        evaluate(state);
        return state;
    }

    state.status = RunStatus::running;
    emit(state, "run.started", state.goal);
    persist(state);

    std::size_t executed_steps = 0;
    while (executed_steps < config_.max_steps) {
        const bool all_complete = std::all_of(
            state.steps.begin(), state.steps.end(), completed);
        if (all_complete) {
            state.status = RunStatus::succeeded;
            const GuardrailDecision output_decision =
                check_guardrails(
                    GuardrailPhase::output, state.output, context);
            if (!output_decision.allowed) {
                state.status = RunStatus::blocked;
                state.error = output_decision.reason;
                emit(state, "run.blocked", state.error);
            } else {
                emit(state, "run.succeeded", state.output);
            }
            persist(state);
            evaluate(state);
            return state;
        }

        StepExecution* ready = nullptr;
        for (StepExecution& candidate : state.steps) {
            if (candidate.status != PlanStepStatus::pending) {
                continue;
            }
            bool dependencies_complete = true;
            for (const std::string& dependency :
                 candidate.step.dependencies) {
                const auto dependency_step = std::find_if(
                    state.steps.begin(),
                    state.steps.end(),
                    [&](const StepExecution& item) {
                        return item.step.id == dependency;
                    });
                dependencies_complete =
                    dependencies_complete &&
                    dependency_step != state.steps.end() &&
                    dependency_step->status ==
                        PlanStepStatus::completed;
            }
            if (dependencies_complete) {
                ready = &candidate;
                break;
            }
        }

        if (ready == nullptr) {
            state.status = RunStatus::failed;
            state.error =
                "no executable plan step remains; a dependency failed";
            emit(state, "run.failed", state.error);
            persist(state);
            evaluate(state);
            return state;
        }

        ready->status = PlanStepStatus::running;
        ++ready->attempts;
        ++executed_steps;
        emit(
            state,
            "step.started",
            ready->step.objective,
            ready->step.id);
        persist(state);

        if (ready->step.requires_approval) {
            if (!approval_handler_) {
                ready->status = PlanStepStatus::blocked;
                ready->error =
                    "step requires approval but no approval handler is configured";
                state.status = RunStatus::blocked;
                state.error = ready->error;
                emit(
                    state,
                    "step.blocked",
                    ready->error,
                    ready->step.id);
                persist(state);
                evaluate(state);
                return state;
            }
            ApprovalRequest request;
            request.run_id = state.run_id;
            request.goal = state.goal;
            request.step = ready->step;
            const ApprovalDecision decision =
                approval_handler_(request);
            if (!decision.approved) {
                ready->status = PlanStepStatus::blocked;
                ready->error = decision.reason.empty()
                    ? "step approval denied"
                    : decision.reason;
                state.status = RunStatus::blocked;
                state.error = ready->error;
                emit(
                    state,
                    "step.blocked",
                    ready->error,
                    ready->step.id);
                persist(state);
                evaluate(state);
                return state;
            }
            emit(
                state,
                "step.approved",
                decision.reason,
                ready->step.id);
        }

        const std::string call_content =
            ready->step.tool + "\n" + ready->step.arguments_json;
        const GuardrailDecision call_decision =
            check_guardrails(
                GuardrailPhase::tool_call, call_content, context);
        if (!call_decision.allowed) {
            ready->status = PlanStepStatus::blocked;
            ready->error = call_decision.reason;
            state.status = RunStatus::blocked;
            state.error = call_decision.reason;
            emit(
                state,
                "step.blocked",
                call_decision.reason,
                ready->step.id);
            persist(state);
            evaluate(state);
            return state;
        }

        try {
            ToolCall call;
            call.id =
                state.run_id + "-" + ready->step.id + "-" +
                std::to_string(ready->attempts);
            call.name = ready->step.tool;
            call.arguments_json = ready->step.arguments_json;
            context.agent_id = ready->step.agent_id.empty()
                ? config_.agent_id
                : ready->step.agent_id;
            if (!ready->step.agent_id.empty()) {
                emit(
                    state,
                    "step.handoff",
                    context.agent_id,
                    ready->step.id);
            }
            ready->result = tools_->invoke(call, context);
            if (ready->result.is_error) {
                throw std::runtime_error(ready->result.content);
            }
            const GuardrailDecision result_decision =
                check_guardrails(
                    GuardrailPhase::tool_result,
                    ready->result.content,
                    context);
            if (!result_decision.allowed) {
                ready->status = PlanStepStatus::blocked;
                ready->error = result_decision.reason;
                state.status = RunStatus::blocked;
                state.error = result_decision.reason;
                emit(
                    state,
                    "step.blocked",
                    result_decision.reason,
                    ready->step.id);
                persist(state);
                evaluate(state);
                return state;
            }
            ready->status = PlanStepStatus::completed;
            state.output = ready->result.content;
            emit(
                state,
                "step.completed",
                ready->result.content,
                ready->step.id);
        } catch (const std::exception& error) {
            ready->error = error.what();
            if (ready->attempts < ready->step.max_attempts) {
                ready->status = PlanStepStatus::pending;
                emit(
                    state,
                    "step.retrying",
                    ready->error,
                    ready->step.id);
            } else {
                ready->status = PlanStepStatus::failed;
                emit(
                    state,
                    "step.failed",
                    ready->error,
                    ready->step.id);
                if (config_.stop_on_error) {
                    state.status = RunStatus::failed;
                    state.error = ready->error;
                    persist(state);
                    evaluate(state);
                    return state;
                }
            }
        }
        persist(state);
    }

    state.status = RunStatus::failed;
    state.error = "agent runtime step budget exhausted";
    emit(state, "run.failed", state.error);
    persist(state);
    evaluate(state);
    return state;
}

} // namespace agent_runtime
} // namespace deep_learning
} // namespace ml
