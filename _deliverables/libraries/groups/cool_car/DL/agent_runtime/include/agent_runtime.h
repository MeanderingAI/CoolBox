#ifndef ML_DEEP_LEARNING_AGENT_RUNTIME_H
#define ML_DEEP_LEARNING_AGENT_RUNTIME_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agent_runtime {

enum class MemoryKind {
    working,
    episodic,
    semantic,
    procedural
};

struct MemoryRecord {
    std::string id;
    std::string name_space = "default";
    MemoryKind kind = MemoryKind::working;
    std::string content;
    std::map<std::string, std::string> metadata;
    double importance = 0.5;
    std::uint64_t created_at_ms = 0;
    std::uint64_t expires_at_ms = 0;
};

struct MemoryQuery {
    std::string name_space = "default";
    std::vector<MemoryKind> kinds;
    std::string text;
    std::size_t limit = 8;
    double minimum_score = 0.0;
};

struct MemoryMatch {
    MemoryRecord record;
    double score = 0.0;
};

class MemoryStore {
public:
    virtual ~MemoryStore() = default;
    virtual std::string remember(MemoryRecord record) = 0;
    virtual std::vector<MemoryMatch> recall(const MemoryQuery& query) const = 0;
    virtual bool forget(const std::string& id) = 0;
    virtual void clear(const std::string& name_space) = 0;
};

class InMemoryMemoryStore : public MemoryStore {
public:
    std::string remember(MemoryRecord record) override;
    std::vector<MemoryMatch> recall(const MemoryQuery& query) const override;
    bool forget(const std::string& id) override;
    void clear(const std::string& name_space) override;
    std::size_t size() const { return records_.size(); }

private:
    std::unordered_map<std::string, MemoryRecord> records_;
    std::uint64_t next_id_ = 1;
};

struct ExecutionContext {
    std::string run_id;
    std::string agent_id = "default";
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> credentials;
    std::shared_ptr<MemoryStore> memory;
};

struct ToolDefinition {
    std::string name;
    std::string description;
    std::string parameters_json_schema =
        R"({"type":"object","properties":{},"additionalProperties":false})";
    bool strict = true;
    std::vector<std::string> required_credentials;
    bool sandbox_required = false;
};

struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments_json = "{}";
};

struct ToolResult {
    std::string call_id;
    std::string name;
    std::string content;
    bool is_error = false;
};

using ToolHandler =
    std::function<ToolResult(const ToolCall&, ExecutionContext&)>;

class CredentialProvider {
public:
    virtual ~CredentialProvider() = default;
    virtual std::optional<std::string> resolve(
        const std::string& name) const = 0;
};

class EnvironmentCredentialProvider : public CredentialProvider {
public:
    explicit EnvironmentCredentialProvider(
        std::vector<std::string> allowed_names);
    std::optional<std::string> resolve(
        const std::string& name) const override;

private:
    std::vector<std::string> allowed_names_;
};

struct ToolPolicyDecision {
    bool allowed = true;
    std::string reason;
};

using ToolExecutionPolicy = std::function<ToolPolicyDecision(
    const ToolDefinition&,
    const ToolCall&,
    const ExecutionContext&)>;

class ToolRegistry {
public:
    void register_tool(ToolDefinition definition, ToolHandler handler);
    bool remove_tool(const std::string& name);
    bool contains(const std::string& name) const;
    std::vector<ToolDefinition> definitions() const;
    ToolResult invoke(const ToolCall& call, ExecutionContext& context) const;
    void set_credential_provider(
        std::shared_ptr<CredentialProvider> provider);
    void set_execution_policy(ToolExecutionPolicy policy);

    // Schema accepted by OpenAI Chat Completions and Responses APIs.
    std::string openai_tools_json() const;

private:
    struct RegisteredTool {
        ToolDefinition definition;
        ToolHandler handler;
    };
    std::map<std::string, RegisteredTool> tools_;
    std::shared_ptr<CredentialProvider> credential_provider_;
    ToolExecutionPolicy execution_policy_;
};

enum class PlanStepStatus {
    pending,
    running,
    completed,
    failed,
    blocked
};

struct PlanStep {
    std::string id;
    std::string objective;
    std::string tool;
    std::string arguments_json = "{}";
    std::vector<std::string> dependencies;
    std::size_t max_attempts = 1;
    bool requires_approval = false;
    std::string agent_id;
    int worker_rank = -1;
};

struct Plan {
    std::vector<PlanStep> steps;
    void validate() const;
};

enum class RunStatus {
    pending,
    running,
    succeeded,
    failed,
    blocked
};

struct StepExecution {
    PlanStep step;
    PlanStepStatus status = PlanStepStatus::pending;
    std::size_t attempts = 0;
    ToolResult result;
    std::string error;
};

struct AgentEvent {
    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ms = 0;
    std::string run_id;
    std::string type;
    std::string step_id;
    std::string message;
    std::map<std::string, std::string> attributes;
};

class EventSink {
public:
    virtual ~EventSink() = default;
    virtual void emit(const AgentEvent& event) = 0;
};

class InMemoryEventSink : public EventSink {
public:
    void emit(const AgentEvent& event) override;
    const std::vector<AgentEvent>& events() const { return events_; }
    void clear() { events_.clear(); }

private:
    std::vector<AgentEvent> events_;
};

struct EvaluationRecord {
    std::string run_id;
    std::string evaluator;
    double score = 0.0;
    bool passed = false;
    std::string feedback;
    std::map<std::string, double> metrics;
};

class EvaluationSink {
public:
    virtual ~EvaluationSink() = default;
    virtual void record(const EvaluationRecord& evaluation) = 0;
};

class InMemoryEvaluationSink : public EvaluationSink {
public:
    void record(const EvaluationRecord& evaluation) override;
    const std::vector<EvaluationRecord>& evaluations() const {
        return evaluations_;
    }

private:
    std::vector<EvaluationRecord> evaluations_;
};

enum class GuardrailPhase {
    input,
    tool_call,
    tool_result,
    output
};

struct GuardrailDecision {
    bool allowed = true;
    std::string reason;
};

using Guardrail = std::function<GuardrailDecision(
    GuardrailPhase, const std::string&, const ExecutionContext&)>;

struct ApprovalRequest {
    std::string run_id;
    std::string goal;
    PlanStep step;
};

struct ApprovalDecision {
    bool approved = false;
    std::string reason;
};

using ApprovalHandler =
    std::function<ApprovalDecision(const ApprovalRequest&)>;

struct AgentRunState {
    std::string run_id;
    std::string goal;
    RunStatus status = RunStatus::pending;
    std::vector<StepExecution> steps;
    std::string output;
    std::string error;
    std::uint64_t next_event_sequence = 1;
};

class RunStateStore {
public:
    virtual ~RunStateStore() = default;
    virtual void save(const AgentRunState& state) = 0;
    virtual std::optional<AgentRunState> load(const std::string& run_id) const = 0;
};

class InMemoryRunStateStore : public RunStateStore {
public:
    void save(const AgentRunState& state) override;
    std::optional<AgentRunState> load(const std::string& run_id) const override;

private:
    std::unordered_map<std::string, AgentRunState> states_;
};

using Evaluator =
    std::function<EvaluationRecord(const AgentRunState&)>;
using Planner =
    std::function<Plan(const std::string&, const std::vector<MemoryMatch>&)>;

struct AgentRuntimeConfig {
    std::size_t max_steps = 64;
    bool stop_on_error = true;
    std::string agent_id = "default";
    std::string memory_name_space = "default";
};

class AgentRuntime {
public:
    AgentRuntime(
        std::shared_ptr<ToolRegistry> tools,
        std::shared_ptr<MemoryStore> memory =
            std::make_shared<InMemoryMemoryStore>(),
        std::shared_ptr<RunStateStore> states =
            std::make_shared<InMemoryRunStateStore>(),
        std::shared_ptr<EventSink> events =
            std::make_shared<InMemoryEventSink>(),
        std::shared_ptr<EvaluationSink> evaluations =
            std::make_shared<InMemoryEvaluationSink>(),
        AgentRuntimeConfig config = AgentRuntimeConfig());

    void add_guardrail(Guardrail guardrail);
    void add_evaluator(Evaluator evaluator);
    void set_approval_handler(ApprovalHandler handler);

    AgentRunState run(const std::string& goal, const Plan& plan);
    AgentRunState run_with_planner(
        const std::string& goal, const Planner& planner);
    AgentRunState resume(const std::string& run_id);

private:
    AgentRunState execute(AgentRunState state);
    GuardrailDecision check_guardrails(
        GuardrailPhase phase,
        const std::string& content,
        const ExecutionContext& context) const;
    void emit(
        AgentRunState& state,
        const std::string& type,
        const std::string& message,
        const std::string& step_id = "");
    void persist(const AgentRunState& state);
    void evaluate(const AgentRunState& state);

    std::shared_ptr<ToolRegistry> tools_;
    std::shared_ptr<MemoryStore> memory_;
    std::shared_ptr<RunStateStore> states_;
    std::shared_ptr<EventSink> events_;
    std::shared_ptr<EvaluationSink> evaluations_;
    AgentRuntimeConfig config_;
    std::vector<Guardrail> guardrails_;
    std::vector<Evaluator> evaluators_;
    ApprovalHandler approval_handler_;
    std::uint64_t next_run_id_ = 1;
};

} // namespace agent_runtime
} // namespace deep_learning
} // namespace ml

#endif
