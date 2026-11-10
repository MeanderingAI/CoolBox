#include "distributed_agent_runtime.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace agent_runtime {
namespace {

std::string required_metadata(
    const distributed::Message& message,
    const std::string& key) {
    const auto iterator = message.metadata.find(key);
    if (iterator == message.metadata.end()) {
        throw std::invalid_argument(
            "distributed message is missing metadata: " + key);
    }
    return iterator->second;
}

bool all_steps_completed(const AgentRunState& state) {
    return std::all_of(
        state.steps.begin(),
        state.steps.end(),
        [](const StepExecution& step) {
            return step.status == PlanStepStatus::completed;
        });
}

bool dependencies_completed(
    const AgentRunState& state,
    const StepExecution& candidate) {
    for (const std::string& dependency :
         candidate.step.dependencies) {
        const auto iterator = std::find_if(
            state.steps.begin(),
            state.steps.end(),
            [&](const StepExecution& step) {
                return step.step.id == dependency;
            });
        if (iterator == state.steps.end() ||
            iterator->status != PlanStepStatus::completed) {
            return false;
        }
    }
    return true;
}

std::uint64_t current_time_ms() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

} // namespace

AgentWorker::AgentWorker(
    std::shared_ptr<distributed::DistributedContext> context,
    std::shared_ptr<ToolRegistry> tools)
    : context_(std::move(context)), tools_(std::move(tools)) {
    if (!context_ || !tools_) {
        throw std::invalid_argument(
            "worker context and tool registry cannot be null");
    }
    if (context_->rank() == 0) {
        throw std::invalid_argument(
            "rank zero is reserved for the coordinator");
    }
}

AgentWorker::~AgentWorker() {
    stop();
    join();
}

void AgentWorker::start() {
    if (running_.exchange(true)) {
        throw std::logic_error("worker is already running");
    }
    thread_ = std::thread(&AgentWorker::loop, this);
}

void AgentWorker::stop() {
    if (!running_) {
        return;
    }
    distributed::Message message(
        distributed::MessageType::COMMAND,
        context_->rank(),
        context_->rank());
    message.metadata["action"] = "shutdown";
    context_->send(message, context_->rank());
}

void AgentWorker::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AgentWorker::loop() {
    while (running_) {
        distributed::Message command;
        if (!context_->receive_for(command, 50)) {
            continue;
        }
        if (command.type != distributed::MessageType::COMMAND) {
            continue;
        }
        const std::string action =
            required_metadata(command, "action");
        if (action == "shutdown") {
            running_ = false;
            return;
        }
        if (action != "execute") {
            continue;
        }

        distributed::Message response(
            distributed::MessageType::RESULT,
            context_->rank(),
            command.source_rank);
        response.metadata["run_id"] =
            required_metadata(command, "run_id");
        response.metadata["step_id"] =
            required_metadata(command, "step_id");
        response.metadata["attempt"] =
            required_metadata(command, "attempt");
        response.metadata["call_id"] =
            required_metadata(command, "call_id");
        try {
            ToolCall call;
            call.id = required_metadata(command, "call_id");
            call.name = required_metadata(command, "tool");
            call.arguments_json =
                required_metadata(command, "arguments");
            ExecutionContext context;
            context.run_id = response.metadata["run_id"];
            context.agent_id =
                required_metadata(command, "agent_id");
            const ToolResult result = tools_->invoke(call, context);
            response.metadata["status"] =
                result.is_error ? "error" : "ok";
            response.metadata["content"] = result.content;
        } catch (const std::exception& error) {
            response.metadata["status"] = "error";
            response.metadata["content"] = error.what();
        }
        context_->send(response, command.source_rank);
    }
}

DistributedAgentRuntime::DistributedAgentRuntime(
    std::shared_ptr<distributed::DistributedContext> coordinator,
    std::shared_ptr<ToolRegistry> tools,
    std::shared_ptr<MemoryStore> memory,
    std::shared_ptr<RunStateStore> states,
    std::shared_ptr<EventSink> events,
    std::shared_ptr<EvaluationSink> evaluations,
    DistributedRuntimeConfig config)
    : coordinator_(std::move(coordinator)),
      tools_(std::move(tools)),
      memory_(std::move(memory)),
      states_(std::move(states)),
      events_(std::move(events)),
      evaluations_(std::move(evaluations)),
      config_(std::move(config)) {
    if (!coordinator_ || !tools_ || !memory_ || !states_ ||
        !events_ || !evaluations_) {
        throw std::invalid_argument(
            "distributed runtime dependencies cannot be null");
    }
    if (coordinator_->rank() != 0 ||
        coordinator_->world_size() < 2) {
        throw std::invalid_argument(
            "distributed runtime requires rank-zero coordinator and at least one worker");
    }
    if (config_.max_steps == 0 ||
        config_.task_timeout_ms <= 0 ||
        config_.receive_poll_ms <= 0) {
        throw std::invalid_argument(
            "distributed runtime limits must be positive");
    }
}

void DistributedAgentRuntime::add_guardrail(
    Guardrail guardrail) {
    if (!guardrail) {
        throw std::invalid_argument("guardrail cannot be empty");
    }
    guardrails_.push_back(std::move(guardrail));
}

void DistributedAgentRuntime::add_evaluator(
    Evaluator evaluator) {
    if (!evaluator) {
        throw std::invalid_argument("evaluator cannot be empty");
    }
    evaluators_.push_back(std::move(evaluator));
}

void DistributedAgentRuntime::set_approval_handler(
    ApprovalHandler handler) {
    approval_handler_ = std::move(handler);
}

AgentRunState DistributedAgentRuntime::run(
    const std::string& goal, const Plan& plan) {
    if (goal.empty()) {
        throw std::invalid_argument("agent goal cannot be empty");
    }
    plan.validate();
    AgentRunState state;
    state.run_id =
        "distributed-run-" + std::to_string(next_run_id_++);
    state.goal = goal;
    for (const PlanStep& step : plan.steps) {
        StepExecution execution;
        execution.step = step;
        state.steps.push_back(std::move(execution));
    }
    return execute(std::move(state));
}

AgentRunState DistributedAgentRuntime::run_with_planner(
    const std::string& goal, const Planner& planner) {
    if (!planner) {
        throw std::invalid_argument("planner cannot be empty");
    }
    MemoryQuery query;
    query.text = goal;
    return run(goal, planner(goal, memory_->recall(query)));
}

AgentRunState DistributedAgentRuntime::resume(
    const std::string& run_id) {
    const auto loaded = states_->load(run_id);
    if (!loaded.has_value()) {
        throw std::invalid_argument("unknown run id: " + run_id);
    }
    AgentRunState state = *loaded;
    for (StepExecution& step : state.steps) {
        if (step.status == PlanStepStatus::running) {
            step.status = PlanStepStatus::pending;
        }
    }
    return execute(std::move(state));
}

void DistributedAgentRuntime::cancel(
    const std::string& run_id) {
    std::lock_guard<std::mutex> lock(cancellation_mutex_);
    cancelled_runs_.insert(run_id);
}

bool DistributedAgentRuntime::is_cancelled(
    const std::string& run_id) const {
    std::lock_guard<std::mutex> lock(cancellation_mutex_);
    return cancelled_runs_.count(run_id) != 0;
}

GuardrailDecision DistributedAgentRuntime::check_guardrails(
    GuardrailPhase phase,
    const std::string& content,
    const ExecutionContext& context) const {
    for (const Guardrail& guardrail : guardrails_) {
        const GuardrailDecision decision =
            guardrail(phase, content, context);
        if (!decision.allowed) {
            return decision;
        }
    }
    return {};
}

ApprovalDecision DistributedAgentRuntime::approve(
    const AgentRunState& state, const PlanStep& step) const {
    if (!step.requires_approval) {
        return {true, ""};
    }
    if (!approval_handler_) {
        return {
            false,
            "step requires approval but no approval handler is configured"};
    }
    return approval_handler_(
        ApprovalRequest{state.run_id, state.goal, step});
}

void DistributedAgentRuntime::emit(
    AgentRunState& state,
    const std::string& type,
    const std::string& message,
    const std::string& step_id,
    int worker_rank) {
    AgentEvent event;
    event.sequence = state.next_event_sequence++;
    event.timestamp_ms = current_time_ms();
    event.run_id = state.run_id;
    event.type = type;
    event.step_id = step_id;
    event.message = message;
    if (worker_rank >= 0) {
        event.attributes["worker_rank"] =
            std::to_string(worker_rank);
    }
    events_->emit(event);
}

void DistributedAgentRuntime::persist(
    const AgentRunState& state) {
    states_->save(state);
}

void DistributedAgentRuntime::evaluate(
    const AgentRunState& state) {
    for (const Evaluator& evaluator : evaluators_) {
        EvaluationRecord record = evaluator(state);
        if (record.run_id.empty()) {
            record.run_id = state.run_id;
        }
        evaluations_->record(record);
    }
}

AgentRunState DistributedAgentRuntime::execute(
    AgentRunState state) {
    ExecutionContext context;
    context.run_id = state.run_id;
    context.agent_id = config_.agent_id;
    context.memory = memory_;
    const GuardrailDecision input =
        check_guardrails(GuardrailPhase::input, state.goal, context);
    if (!input.allowed) {
        state.status = RunStatus::blocked;
        state.error = input.reason;
        emit(state, "run.blocked", state.error);
        persist(state);
        evaluate(state);
        return state;
    }

    state.status = RunStatus::running;
    emit(state, "run.started", state.goal);
    persist(state);

    std::vector<int> idle_workers;
    for (int rank = 1; rank < coordinator_->world_size(); ++rank) {
        idle_workers.push_back(rank);
    }
    std::map<int, ActiveTask> active;
    std::size_t dispatch_count = 0;

    while (dispatch_count < config_.max_steps || !active.empty()) {
        if (is_cancelled(state.run_id)) {
            state.status = RunStatus::failed;
            state.error = "run cancelled";
            emit(state, "run.cancelled", state.error);
            persist(state);
            evaluate(state);
            return state;
        }
        if (all_steps_completed(state)) {
            const GuardrailDecision output =
                check_guardrails(
                    GuardrailPhase::output, state.output, context);
            if (!output.allowed) {
                state.status = RunStatus::blocked;
                state.error = output.reason;
                emit(state, "run.blocked", state.error);
            } else {
                state.status = RunStatus::succeeded;
                emit(state, "run.succeeded", state.output);
            }
            persist(state);
            evaluate(state);
            return state;
        }

        bool dispatched = false;
        for (std::size_t index = 0;
             index < state.steps.size() &&
             !idle_workers.empty() &&
             dispatch_count < config_.max_steps;
             ++index) {
            StepExecution& execution = state.steps[index];
            if (execution.status != PlanStepStatus::pending ||
                !dependencies_completed(state, execution)) {
                continue;
            }
            auto worker = idle_workers.begin();
            if (execution.step.worker_rank >= 1) {
                worker = std::find(
                    idle_workers.begin(),
                    idle_workers.end(),
                    execution.step.worker_rank);
                if (worker == idle_workers.end()) {
                    continue;
                }
            }
            const ApprovalDecision approval =
                approve(state, execution.step);
            if (!approval.approved) {
                execution.status = PlanStepStatus::blocked;
                execution.error = approval.reason;
                state.status = RunStatus::blocked;
                state.error = approval.reason;
                emit(
                    state,
                    "step.blocked",
                    state.error,
                    execution.step.id);
                persist(state);
                evaluate(state);
                return state;
            }
            const std::string call_content =
                execution.step.tool + "\n" +
                execution.step.arguments_json;
            const GuardrailDecision call_guardrail =
                check_guardrails(
                    GuardrailPhase::tool_call,
                    call_content,
                    context);
            if (!call_guardrail.allowed) {
                execution.status = PlanStepStatus::blocked;
                execution.error = call_guardrail.reason;
                state.status = RunStatus::blocked;
                state.error = call_guardrail.reason;
                emit(
                    state,
                    "step.blocked",
                    state.error,
                    execution.step.id);
                persist(state);
                evaluate(state);
                return state;
            }

            const int rank = *worker;
            idle_workers.erase(worker);
            execution.status = PlanStepStatus::running;
            ++execution.attempts;
            ++dispatch_count;
            distributed::Message command(
                distributed::MessageType::COMMAND, 0, rank);
            command.metadata["action"] = "execute";
            command.metadata["run_id"] = state.run_id;
            command.metadata["step_id"] = execution.step.id;
            command.metadata["attempt"] =
                std::to_string(execution.attempts);
            command.metadata["call_id"] =
                state.run_id + "-" + execution.step.id + "-" +
                std::to_string(execution.attempts);
            command.metadata["tool"] = execution.step.tool;
            command.metadata["arguments"] =
                execution.step.arguments_json;
            command.metadata["agent_id"] =
                execution.step.agent_id.empty()
                    ? config_.agent_id
                    : execution.step.agent_id;
            coordinator_->send(command, rank);
            active[rank] = ActiveTask{
                index,
                execution.attempts,
                std::chrono::steady_clock::now()};
            emit(
                state,
                "step.dispatched",
                execution.step.objective,
                execution.step.id,
                rank);
            dispatched = true;
        }
        if (dispatched) {
            persist(state);
        }

        distributed::Message response;
        if (coordinator_->receive_for(
                response, config_.receive_poll_ms)) {
            const int rank = response.source_rank;
            const auto task = active.find(rank);
            if (response.type != distributed::MessageType::RESULT ||
                task == active.end() ||
                required_metadata(response, "run_id") !=
                    state.run_id ||
                required_metadata(response, "step_id") !=
                    state.steps[task->second.step_index].step.id) {
                continue;
            }
            StepExecution& execution =
                state.steps[task->second.step_index];
            const std::size_t attempt = static_cast<std::size_t>(
                std::stoull(required_metadata(response, "attempt")));
            if (attempt != task->second.attempt) {
                continue;
            }
            active.erase(task);
            idle_workers.push_back(rank);
            const std::string content =
                required_metadata(response, "content");
            if (required_metadata(response, "status") == "ok") {
                const GuardrailDecision result_guardrail =
                    check_guardrails(
                        GuardrailPhase::tool_result,
                        content,
                        context);
                if (!result_guardrail.allowed) {
                    execution.status = PlanStepStatus::blocked;
                    execution.error = result_guardrail.reason;
                    state.status = RunStatus::blocked;
                    state.error = result_guardrail.reason;
                    emit(
                        state,
                        "step.blocked",
                        state.error,
                        execution.step.id,
                        rank);
                    persist(state);
                    evaluate(state);
                    return state;
                }
                execution.status = PlanStepStatus::completed;
                execution.result.call_id =
                    required_metadata(response, "call_id");
                execution.result.name = execution.step.tool;
                execution.result.content = content;
                state.output = content;
                emit(
                    state,
                    "step.completed",
                    content,
                    execution.step.id,
                    rank);
            } else if (
                execution.attempts <
                execution.step.max_attempts) {
                execution.status = PlanStepStatus::pending;
                execution.error = content;
                emit(
                    state,
                    "step.retrying",
                    content,
                    execution.step.id,
                    rank);
            } else {
                execution.status = PlanStepStatus::failed;
                execution.error = content;
                emit(
                    state,
                    "step.failed",
                    content,
                    execution.step.id,
                    rank);
                if (config_.stop_on_error) {
                    state.status = RunStatus::failed;
                    state.error = content;
                    persist(state);
                    evaluate(state);
                    return state;
                }
            }
            persist(state);
        }

        const auto now = std::chrono::steady_clock::now();
        for (auto iterator = active.begin();
             iterator != active.end();) {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - iterator->second.started)
                    .count();
            if (elapsed <= config_.task_timeout_ms) {
                ++iterator;
                continue;
            }
            const int rank = iterator->first;
            StepExecution& execution =
                state.steps[iterator->second.step_index];
            execution.error = "distributed tool execution timed out";
            if (execution.attempts < execution.step.max_attempts) {
                execution.status = PlanStepStatus::pending;
                emit(
                    state,
                    "step.retrying",
                    execution.error,
                    execution.step.id,
                    rank);
            } else {
                execution.status = PlanStepStatus::failed;
                emit(
                    state,
                    "step.failed",
                    execution.error,
                    execution.step.id,
                    rank);
                if (config_.stop_on_error) {
                    state.status = RunStatus::failed;
                    state.error = execution.error;
                    persist(state);
                    evaluate(state);
                    return state;
                }
            }
            idle_workers.push_back(rank);
            iterator = active.erase(iterator);
            persist(state);
        }

        if (active.empty()) {
            const bool any_ready = std::any_of(
                state.steps.begin(),
                state.steps.end(),
                [&](const StepExecution& step) {
                    return step.status == PlanStepStatus::pending &&
                           dependencies_completed(state, step);
                });
            if (!any_ready && !all_steps_completed(state)) {
                state.status = RunStatus::failed;
                state.error =
                    "no executable distributed plan step remains";
                emit(state, "run.failed", state.error);
                persist(state);
                evaluate(state);
                return state;
            }
        }
    }

    if (all_steps_completed(state)) {
        const GuardrailDecision output =
            check_guardrails(
                GuardrailPhase::output, state.output, context);
        if (!output.allowed) {
            state.status = RunStatus::blocked;
            state.error = output.reason;
            emit(state, "run.blocked", state.error);
        } else {
            state.status = RunStatus::succeeded;
            emit(state, "run.succeeded", state.output);
        }
    } else {
        state.status = RunStatus::failed;
        state.error = "distributed runtime step budget exhausted";
        emit(state, "run.failed", state.error);
    }
    persist(state);
    evaluate(state);
    return state;
}

} // namespace agent_runtime
} // namespace deep_learning
} // namespace ml
