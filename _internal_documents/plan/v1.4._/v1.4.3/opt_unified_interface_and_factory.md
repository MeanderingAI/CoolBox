# opt_unified_interface_and_factory

## Scope
- Introduce a shared optimization interface for all OPT algorithms.
- Make genetic_search, simulated_annealing, and hill_climbing conform to the interface.
- Add factory module for enum or config driven optimizer creation.
- Keep existing convenience APIs where useful.

## Shared Interface
- Added optimization_algorithm.h with:
  - ObjectiveFunction alias.
  - OptimizationAlgorithm base class.
  - Virtual methods:
    - optimize(objective, initial_state)
    - best_solution()
    - best_score()

## Conformance Changes

### Genetic Search
- Now derives from OptimizationAlgorithm.
- Added interface override accepting initial_state.
- Preserved convenience overload optimize(objective).
- Initial state is injected into initial population when provided.

### Simulated Annealing
- Now derives from OptimizationAlgorithm.
- Uses shared ObjectiveFunction signature.

### Hill Climbing
- Now derives from OptimizationAlgorithm.
- Uses shared ObjectiveFunction signature.
- Added polymorphic test usage through base pointer.

## Factory Module
- Added optimization_factory submodule under OPT.
- Factory API includes:
  - OptimizationType enum.
  - conversion helpers: from string and to string.
  - create_optimizer by enum.
  - create_optimizer overloads by algorithm-specific config.
- Factory target links to:
  - genetic_search
  - simulated_annealing
  - hill_climbing

## Build Wiring
- Added shared OPT include path to all optimizer targets so interface headers are visible.
- Added optimization_factory target and conditional tyst test target.

## Validation
- CMake reconfiguration confirms optimization_factory is discovered under OPT.
- Successful target build:
  - optimization_factory
- Diagnostics on new factory and interface files report no errors.

## Result
- Callers can choose algorithm strategies with a single unified contract and a simple factory entry point.
