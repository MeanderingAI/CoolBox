# temporal_bayesian_networks

## Scope
- Add Dynamic Bayesian Network implementation to the ML package.
- Add Continuous Time Bayesian Network implementation to the ML package.
- Provide practical APIs for prediction/propagation with normalized probability outputs.
- Include build wiring and tests for each module.

## Modules Added
- dynamic_bayesian_network under cool_car ML group.
- continuous_time_bayesian_network under cool_car ML group.

## Architecture

### Dynamic Bayesian Network
- Maintains state labels, initial distribution, and transition matrix.
- Provides one-step prediction from a belief vector.
- Provides evidence update with posterior normalization.
- Provides forward filtering over a sequence of likelihood vectors.

### Continuous Time Bayesian Network
- Maintains state labels and an intensity matrix Q.
- Validates CTMC constraints on Q:
  - Off-diagonal values are non-negative.
  - Each diagonal equals negative sum of outgoing rates.
- Computes transition matrix P(dt) via truncated matrix-exponential series.
- Supports belief propagation for one step and trajectory propagation for multiple steps.

## Build Wiring
- Added dedicated CMake submodules for each implementation.
- Both modules are discoverable through ML package auto-subdirectory scanning.
- Tests are defined conditionally when BUILD_TESTING is enabled and tyst framework target is available.

## API Surface

### Dynamic Bayesian Network
- set_states(states)
- set_initial_distribution(distribution)
- set_transition_matrix(matrix)
- predict_next(belief)
- update_with_evidence(predicted_belief, evidence_likelihood)
- forward_filter(evidence_likelihoods)

### Continuous Time Bayesian Network
- set_states(states)
- set_intensity_matrix(Q)
- transition_matrix(dt, series_terms)
- propagate(belief, dt, series_terms)
- propagate_trajectory(initial_belief, step_durations, series_terms)

## Validation
- CMake reconfiguration confirms ML subdirectories include both new modules.
- Successful target builds:
  - dynamic_bayesian_network
  - continuous_time_bayesian_network
- Editor diagnostics on new module files show no errors.

## Notes
- Test targets are conditionally generated and may be absent when tyst framework target is unavailable in the current build configuration.
