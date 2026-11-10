import asyncio
import json
import os
import sqlite3
import subprocess
import threading
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path

try:
    from . import ml_core
except ImportError:
    import ml_core


native = ml_core.deep_learning.agent_runtime


def _enum_name(value):
    return value.name


def _tool_result_to_dict(result):
    return {
        "call_id": result.call_id,
        "name": result.name,
        "content": result.content,
        "is_error": result.is_error,
    }


def _step_to_dict(execution):
    step = execution.step
    return {
        "step": {
            "id": step.id,
            "objective": step.objective,
            "tool": step.tool,
            "arguments_json": step.arguments_json,
            "dependencies": list(step.dependencies),
            "max_attempts": step.max_attempts,
            "requires_approval": step.requires_approval,
            "agent_id": step.agent_id,
            "worker_rank": step.worker_rank,
        },
        "status": _enum_name(execution.status),
        "attempts": execution.attempts,
        "result": _tool_result_to_dict(execution.result),
        "error": execution.error,
    }


def _state_to_json(state):
    return json.dumps(
        {
            "run_id": state.run_id,
            "goal": state.goal,
            "status": _enum_name(state.status),
            "steps": [_step_to_dict(step) for step in state.steps],
            "output": state.output,
            "error": state.error,
            "next_event_sequence": state.next_event_sequence,
        },
        separators=(",", ":"),
    )


def _state_from_json(payload):
    value = json.loads(payload)
    state = native.AgentRunState()
    state.run_id = value["run_id"]
    state.goal = value["goal"]
    state.status = getattr(native.RunStatus, value["status"])
    state.output = value["output"]
    state.error = value["error"]
    state.next_event_sequence = value["next_event_sequence"]
    executions = []
    for item in value["steps"]:
        step_value = item["step"]
        step = native.PlanStep()
        step.id = step_value["id"]
        step.objective = step_value["objective"]
        step.tool = step_value["tool"]
        step.arguments_json = step_value["arguments_json"]
        step.dependencies = step_value["dependencies"]
        step.max_attempts = step_value["max_attempts"]
        step.requires_approval = step_value["requires_approval"]
        step.agent_id = step_value["agent_id"]
        step.worker_rank = step_value["worker_rank"]

        result_value = item["result"]
        result = native.ToolResult()
        result.call_id = result_value["call_id"]
        result.name = result_value["name"]
        result.content = result_value["content"]
        result.is_error = result_value["is_error"]

        execution = native.StepExecution()
        execution.step = step
        execution.status = getattr(
            native.PlanStepStatus, item["status"]
        )
        execution.attempts = item["attempts"]
        execution.result = result
        execution.error = item["error"]
        executions.append(execution)
    state.steps = executions
    return state


class SQLiteRunStateStore(native.RunStateStore):
    def __init__(self, path):
        super().__init__()
        self.path = str(path)
        self._lock = threading.RLock()
        self._connection = sqlite3.connect(
            self.path,
            check_same_thread=False,
        )
        with self._connection:
            self._connection.execute("PRAGMA journal_mode=WAL")
            self._connection.execute(
                """
                CREATE TABLE IF NOT EXISTS agent_runs (
                    run_id TEXT PRIMARY KEY,
                    state_json TEXT NOT NULL,
                    updated_at TEXT NOT NULL
                        DEFAULT CURRENT_TIMESTAMP
                )
                """
            )

    def save(self, state):
        payload = _state_to_json(state)
        with self._lock, self._connection:
            self._connection.execute(
                """
                INSERT INTO agent_runs(run_id, state_json, updated_at)
                VALUES (?, ?, CURRENT_TIMESTAMP)
                ON CONFLICT(run_id) DO UPDATE SET
                    state_json = excluded.state_json,
                    updated_at = CURRENT_TIMESTAMP
                """,
                (state.run_id, payload),
            )

    def load(self, run_id):
        with self._lock:
            row = self._connection.execute(
                "SELECT state_json FROM agent_runs WHERE run_id = ?",
                (run_id,),
            ).fetchone()
        return None if row is None else _state_from_json(row[0])

    def close(self):
        with self._lock:
            self._connection.close()


class SQLiteMemoryStore(native.MemoryStore):
    def __init__(self, path):
        super().__init__()
        self.path = str(path)
        self._lock = threading.RLock()
        self._connection = sqlite3.connect(
            self.path,
            check_same_thread=False,
        )
        with self._connection:
            self._connection.execute("PRAGMA journal_mode=WAL")
            self._connection.execute(
                """
                CREATE TABLE IF NOT EXISTS agent_memories (
                    id TEXT PRIMARY KEY,
                    namespace TEXT NOT NULL,
                    kind TEXT NOT NULL,
                    content TEXT NOT NULL,
                    metadata_json TEXT NOT NULL,
                    importance REAL NOT NULL,
                    created_at_ms INTEGER NOT NULL,
                    expires_at_ms INTEGER NOT NULL
                )
                """
            )
            self._connection.execute(
                """
                CREATE INDEX IF NOT EXISTS idx_agent_memories_namespace
                ON agent_memories(namespace, kind)
                """
            )

    def remember(self, record):
        if not record.content:
            raise ValueError("memory content cannot be empty")
        identifier = record.id or f"memory-{uuid.uuid4()}"
        if not 0.0 <= record.importance <= 1.0:
            raise ValueError(
                "memory importance must be between 0 and 1"
            )
        created_at_ms = record.created_at_ms or int(time.time() * 1000)
        with self._lock, self._connection:
            self._connection.execute(
                """
                INSERT INTO agent_memories(
                    id, namespace, kind, content, metadata_json,
                    importance, created_at_ms, expires_at_ms
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    identifier,
                    record.namespace,
                    _enum_name(record.kind),
                    record.content,
                    json.dumps(dict(record.metadata)),
                    record.importance,
                    created_at_ms,
                    record.expires_at_ms,
                ),
            )
        return identifier

    def recall(self, query):
        now_ms = int(time.time() * 1000)
        parameters = [query.namespace, now_ms]
        where = [
            "namespace = ?",
            "(expires_at_ms = 0 OR expires_at_ms > ?)",
        ]
        if query.kinds:
            names = [_enum_name(kind) for kind in query.kinds]
            where.append(
                f"kind IN ({','.join('?' for _ in names)})"
            )
            parameters.extend(names)
        with self._lock:
            rows = self._connection.execute(
                f"""
                SELECT id, namespace, kind, content, metadata_json,
                       importance, created_at_ms, expires_at_ms
                FROM agent_memories
                WHERE {' AND '.join(where)}
                """,
                parameters,
            ).fetchall()
        query_terms = {
            term.lower() for term in query.text.split() if term
        }
        matches = []
        for row in rows:
            record_terms = {
                term.lower() for term in row[3].split() if term
            }
            relevance = (
                len(query_terms & record_terms) / len(query_terms)
                if query_terms
                else row[5]
            )
            score = (
                0.8 * relevance + 0.2 * row[5]
                if query_terms
                else row[5]
            )
            if score < query.minimum_score:
                continue
            record = native.MemoryRecord()
            record.id = row[0]
            record.namespace = row[1]
            record.kind = getattr(native.MemoryKind, row[2])
            record.content = row[3]
            record.metadata = json.loads(row[4])
            record.importance = row[5]
            record.created_at_ms = row[6]
            record.expires_at_ms = row[7]
            match = native.MemoryMatch()
            match.record = record
            match.score = score
            matches.append(match)
        matches.sort(
            key=lambda item: (
                -item.score,
                -item.record.created_at_ms,
                item.record.id,
            )
        )
        return matches[: query.limit]

    def forget(self, identifier):
        with self._lock, self._connection:
            cursor = self._connection.execute(
                "DELETE FROM agent_memories WHERE id = ?",
                (identifier,),
            )
        return cursor.rowcount > 0

    def clear(self, namespace):
        with self._lock, self._connection:
            self._connection.execute(
                "DELETE FROM agent_memories WHERE namespace = ?",
                (namespace,),
            )

    def close(self):
        with self._lock:
            self._connection.close()


class SQLiteEvaluationSink(native.EvaluationSink):
    def __init__(self, path):
        super().__init__()
        self.path = str(path)
        self._lock = threading.RLock()
        self._connection = sqlite3.connect(
            self.path,
            check_same_thread=False,
        )
        with self._connection:
            self._connection.execute("PRAGMA journal_mode=WAL")
            self._connection.execute(
                """
                CREATE TABLE IF NOT EXISTS agent_evaluations (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    run_id TEXT NOT NULL,
                    evaluator TEXT NOT NULL,
                    score REAL NOT NULL,
                    passed INTEGER NOT NULL,
                    feedback TEXT NOT NULL,
                    metrics_json TEXT NOT NULL,
                    created_at TEXT NOT NULL
                        DEFAULT CURRENT_TIMESTAMP
                )
                """
            )

    def record(self, evaluation):
        with self._lock, self._connection:
            self._connection.execute(
                """
                INSERT INTO agent_evaluations(
                    run_id, evaluator, score, passed,
                    feedback, metrics_json
                ) VALUES (?, ?, ?, ?, ?, ?)
                """,
                (
                    evaluation.run_id,
                    evaluation.evaluator,
                    evaluation.score,
                    int(evaluation.passed),
                    evaluation.feedback,
                    json.dumps(dict(evaluation.metrics)),
                ),
            )

    def records(self, run_id):
        with self._lock:
            rows = self._connection.execute(
                """
                SELECT evaluator, score, passed, feedback, metrics_json
                FROM agent_evaluations
                WHERE run_id = ?
                ORDER BY id
                """,
                (run_id,),
            ).fetchall()
        return [
            {
                "evaluator": row[0],
                "score": row[1],
                "passed": bool(row[2]),
                "feedback": row[3],
                "metrics": json.loads(row[4]),
            }
            for row in rows
        ]

    def close(self):
        with self._lock:
            self._connection.close()


def _event_to_dict(event):
    return {
        "sequence": event.sequence,
        "timestamp_ms": event.timestamp_ms,
        "run_id": event.run_id,
        "type": event.type,
        "step_id": event.step_id,
        "message": event.message,
        "attributes": dict(event.attributes),
    }


class JsonLinesEventSink(native.EventSink):
    def __init__(self, path):
        super().__init__()
        self.path = Path(path)
        self._lock = threading.Lock()

    def emit(self, event):
        payload = json.dumps(
            _event_to_dict(event),
            separators=(",", ":"),
        )
        with self._lock:
            with self.path.open("a", encoding="utf-8") as stream:
                stream.write(payload + "\n")


class OpenTelemetryEventSink(native.EventSink):
    def __init__(self, exporter):
        super().__init__()
        if not callable(exporter):
            raise TypeError("exporter must be callable")
        self.exporter = exporter

    def emit(self, event):
        self.exporter(
            {
                "name": event.type,
                "timestamp_unix_nano": event.timestamp_ms * 1_000_000,
                "trace_id": event.run_id,
                "span_id": event.step_id,
                "body": event.message,
                "attributes": dict(event.attributes),
            }
        )


class OpenAIResponsesClient:
    def __init__(
        self,
        model,
        api_key=None,
        endpoint="https://api.openai.com/v1/responses",
        timeout=60.0,
        transport=None,
    ):
        self.model = model
        self.api_key = api_key or os.environ.get("OPENAI_API_KEY")
        if not self.api_key and transport is None:
            raise ValueError(
                "OPENAI_API_KEY or a custom transport is required"
            )
        self.endpoint = endpoint
        self.timeout = timeout
        self.transport = transport or self._http_transport

    def _http_transport(self, payload):
        request = urllib.request.Request(
            self.endpoint,
            data=json.dumps(payload).encode("utf-8"),
            headers={
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
            },
            method="POST",
        )
        try:
            with urllib.request.urlopen(
                request,
                timeout=self.timeout,
            ) as response:
                return json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as error:
            body = error.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"OpenAI Responses API returned {error.code}: {body}"
            ) from error

    def create(self, input_value, tools, previous_response_id=None):
        payload = {
            "model": self.model,
            "input": input_value,
            "tools": tools,
        }
        if previous_response_id:
            payload["previous_response_id"] = previous_response_id
        return self.transport(payload)

    def run_tool_loop(
        self,
        prompt,
        registry,
        context=None,
        max_turns=16,
    ):
        if max_turns < 1:
            raise ValueError("max_turns must be at least one")
        context = context or native.ExecutionContext()
        response = self.create(prompt, registry.openai_tools())
        for _ in range(max_turns):
            calls = [
                item
                for item in response.get("output", [])
                if item.get("type") == "function_call"
            ]
            if not calls:
                return response
            outputs = [
                registry.invoke_openai_response_tool_call(
                    call,
                    context,
                )
                for call in calls
            ]
            response = self.create(
                outputs,
                registry.openai_tools(),
                previous_response_id=response.get("id"),
            )
        raise RuntimeError(
            "OpenAI Responses tool loop exceeded max_turns"
        )

    async def run_tool_loop_async(self, *args, **kwargs):
        return await asyncio.to_thread(
            self.run_tool_loop,
            *args,
            **kwargs,
        )


async def invoke_tools_async(registry, tool_calls, context=None):
    context = context or native.ExecutionContext()

    async def invoke(call):
        local_context = native.ExecutionContext()
        local_context.run_id = context.run_id
        local_context.agent_id = context.agent_id
        local_context.metadata = dict(context.metadata)
        local_context.credentials = dict(context.credentials)
        local_context.memory = context.memory
        return await asyncio.to_thread(
            registry.invoke_openai_response_tool_call,
            call,
            local_context,
        )

    return await asyncio.gather(*(invoke(call) for call in tool_calls))


def make_subprocess_tool_handler(
    executable,
    *,
    timeout=30.0,
    allowed_arguments=None,
    working_directory=None,
):
    executable = str(Path(executable).resolve())
    allowed = (
        None
        if allowed_arguments is None
        else set(allowed_arguments)
    )

    def handler(call, context):
        arguments = json.loads(call.arguments_json)
        command_arguments = arguments.get("args", [])
        if not isinstance(command_arguments, list) or not all(
            isinstance(value, str) for value in command_arguments
        ):
            raise ValueError("subprocess tool args must be strings")
        if allowed is not None:
            denied = [
                value
                for value in command_arguments
                if value not in allowed
            ]
            if denied:
                raise PermissionError(
                    f"subprocess arguments are not allowed: {denied}"
                )
        environment = {
            "PATH": os.environ.get("PATH", ""),
            **dict(context.credentials),
        }
        completed = subprocess.run(
            [executable, *command_arguments],
            input=arguments.get("stdin"),
            text=True,
            capture_output=True,
            cwd=working_directory,
            env=environment,
            timeout=timeout,
            check=False,
            shell=False,
        )
        result = native.ToolResult()
        result.call_id = call.id
        result.name = call.name
        result.content = (
            completed.stdout
            if completed.returncode == 0
            else completed.stderr
        )
        result.is_error = completed.returncode != 0
        return result

    return handler
