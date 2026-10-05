# DL agents

## Reflexion (arXiv:2303.11366)

`reflexion.h` implements the paper's model-independent verbal reinforcement
loop:

| Paper component | CoolBox API |
| --- | --- |
| Actor \(M_a\), policy conditioned on `mem` | `ReflexionActor` |
| Trial trajectory \(\tau_t=[a_0,o_0,\ldots]\) | `ReflexionTrajectory` / `ReflexionStep` |
| Evaluator \(M_e(\tau_t)\) | `ReflexionEvaluator` / `ReflexionEvaluation` |
| Self-Reflection \(M_{sr}\) | `ReflexionSelfReflection` |
| bounded episodic memory, \(\Omega\) | `ReflexionAgent::Config::memory_capacity` |
| iterative Algorithm 1 loop | `ReflexionAgent::run` |

The callbacks deliberately separate orchestration from a particular LLM
provider. No weights or optimizer state are changed: a failed trial is distilled
to text, the oldest reflection is evicted when the memory reaches \(\Omega\),
and the next Actor call receives the resulting memory.

The paper's Algorithm 1 pseudocode repeats the initial trajectory and uses an
`or` termination condition, while its prose says to stop when the Evaluator
passes or the trial budget is exhausted. This implementation follows the prose:
each trial is generated once, termination uses those two stopping conditions,
and reflection is generated only after failure because a passing trial has no
next policy invocation to improve.

`detect_alfworld_failure` also implements the paper's deterministic ALFWorld
self-evaluation triggers: more than three repeated identical
action/observation cycles and more than 30 actions. These thresholds are
configurable for other environments.

### Scope and reproducibility

The implementation is the core framework, not the paper's hosted GPT models,
prompts, ALFWorld/HotPotQA/HumanEval datasets, or reported benchmark
replication. Tests use deterministic callbacks to verify trajectory shape,
verbal-memory policy conditioning, bounded FIFO memory, stopping behavior, and
the ALFWorld heuristics. Consequently, the paper's benchmark percentages are
not claimed as reproduced.

Build and run the focused tests from the repository root:

```sh
cmake --build build --target dl_agents_tests -j2
ctest --test-dir build/DL_agents_build -R DeepLearningAgentTests --output-on-failure
```
