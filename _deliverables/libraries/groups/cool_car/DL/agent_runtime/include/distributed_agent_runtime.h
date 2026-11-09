#ifndef ML_DEEP_LEARNING_DISTRIBUTED_AGENT_RUNTIME_H
#define ML_DEEP_LEARNING_DISTRIBUTED_AGENT_RUNTIME_H

#include "agent_runtime.h"
#include "distributed/message_passing.h"

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agent_runtime {

struct DistributedRuntimeConfig {
    std::size_t max_steps = 256;
    int task_timeout_ms = 30000;
    int receive_poll_ms = 10;
    bool stop_on_error = true;
    std::string agent_id = "coordinator";
};

class AgentWorker {
public:
    AgentWorker(
        std::shared_ptr<distributed::DistributedContext> context,
        std::shared_ptr<ToolRegistry> tools);
    ~AgentWorker();

    void start();
    void stop();
    void join();
    bool running() const { return running_; }
    int rank() const { return context_->rank(); }

private:
    void loop();

    std::shared_ptr<distributed::DistributedContext> context_;
    std::shared_ptr<ToolRegistry> tools_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

class DistributedAgentRuntime {
public:
    DistributedAgentRuntime(
        std::shared_ptr<distributed::DistributedContext> coordinator,
        std::shared_ptr<ToolRegistry> tools,
        std::shared_ptr<MemoryStore> memory =
            std::make_shared<InMemoryMemoryStore>(),
        std::shared_ptr<RunStateStore> states =
            std::make_shared<InMemoryRunStateStore>(),
        std::shared_ptr<EventSink> events =
            std::make_shared<InMemoryEventSink>(),
        std::shared_ptr<EvaluationSink> evaluations =
            std::make_shared<InMemoryEvaluationSink>(),
        DistributedRuntimeConfig config = DistributedRuntimeConfig());

    void add_guardrail(Guardrail guardrail);
    void add_evaluator(Evaluator evaluator);
    void set_approval_handler(ApprovalHandler handler);

    AgentRunState run(const std::string& goal, const Plan& plan);
    AgentRunState run_with_planner(
        const std::string& goal, const Planner& planner);
    AgentRunState resume(const std::string& run_id);
    void cancel(const std::string& run_id);

private:
    struct ActiveTask {
        std::size_t step_index = 0;
        std::size_t attempt = 0;
        std::chrono::steady_clock::time_point started;
    };

    AgentRunState execute(AgentRunState state);
    GuardrailDecision check_guardrails(
        GuardrailPhase phase,
        const std::string& content,
        const ExecutionContext& context) const;
    ApprovalDecision approve(
        const AgentRunState& state,
        const PlanStep& step) const;
    void emit(
        AgentRunState& state,
        const std::string& type,
        const std::string& message,
        const std::string& step_id = "",
        int worker_rank = -1);
    void persist(const AgentRunState& state);
    void evaluate(const AgentRunState& state);
    bool is_cancelled(const std::string& run_id) const;

    std::shared_ptr<distributed::DistributedContext> coordinator_;
    std::shared_ptr<ToolRegistry> tools_;
    std::shared_ptr<MemoryStore> memory_;
    std::shared_ptr<RunStateStore> states_;
    std::shared_ptr<EventSink> events_;
    std::shared_ptr<EvaluationSink> evaluations_;
    DistributedRuntimeConfig config_;
    std::vector<Guardrail> guardrails_;
    std::vector<Evaluator> evaluators_;
    ApprovalHandler approval_handler_;
    mutable std::mutex cancellation_mutex_;
    std::set<std::string> cancelled_runs_;
    std::uint64_t next_run_id_ = 1;
};

} // namespace agent_runtime
} // namespace deep_learning
} // namespace ml

#endif
