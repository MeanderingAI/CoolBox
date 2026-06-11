# opt_metaheuristics_package

## Scope
- Add a new OPT package under cool_car.
- Implement three generic metaheuristic optimization modules:
  - genetic_search
  - simulated_annealing
  - hill_climbing
- Add per-module tests and build wiring.

## Package Structure
- Added OPT package root with automatic subdirectory discovery.
- Added submodules:
  - genetic_search
  - simulated_annealing
  - hill_climbing

## Algorithm Implementations

### Genetic Search
- Population-based minimization.
- Tournament selection.
- Blend crossover.
- Gaussian mutation with bound clamping.
- Elitism by carrying best solution forward each generation.

### Simulated Annealing
- Single-state stochastic search.
- Metropolis acceptance criterion for uphill moves.
- Temperature schedule with exponential cooling.
- Bound clamping for candidate states.

### Hill Climbing
- Greedy local search over sampled neighborhood.
- Multi-restart strategy to mitigate local minima trapping.
- Bound clamping for candidate states.

## Build Wiring
- Registered OPT package in cool_car top-level CMake.
- Added independent library targets for each algorithm module.
- Added test target definitions guarded by BUILD_TESTING and tyst target availability.

## Validation
- CMake reconfiguration confirms OPT package and all algorithm submodules are discovered.
- Successful target builds:
  - genetic_search
  - simulated_annealing
  - hill_climbing
- Diagnostics on new implementation files report no errors.
