# Native Robot Control

`robot_navigation` is a static C++17 library for known-map planar localization
and lidar-aware local driving. It reuses the tracker EKF and configurable
sequential Monte Carlo implementations without copying their algorithms or
requiring an additional runtime DLL.

## Localization And Planning

`RobotNavigation` accepts axis-aligned map boxes, relative lidar angles in
radians, maximum range, and the sensor's forward offset. Coordinates are
metres on the X/Z plane; heading zero faces +Z and positive heading turns
toward +X. `predict(distance, yawDelta)` consumes wheel-odometry increments.
`observe(ranges, forwardVector)` consumes ranges and an orientation vector.
`pose()` returns the selected EKF or particle estimate.

The EKF uses a nonlinear map-ray measurement model and numerical Jacobians.
The particle adapter injects its odometry/noise transition and map-range
log likelihood into the existing tracker filter. Robot Lab uses 192 seeded
particles. This requires a known map and approximate initial pose, not SLAM.

`plan(goal, ranges, currentSpeed, currentTurnRate, speedLimit)` implements
Dynamic Window-style trajectory sampling with velocity limits, footprint
clearance, braking clearance, heading/progress scoring, and a conservative
stop on invalid scans. The returned `DrivePlan` includes the command, sampled
path, directional-clearance mask, and blocked status. Sparse lidar and local
planning do not guarantee global reachability or detect every unseen obstacle.

## Configurable Particle Filter

`SequentialMonteCarlo(initialStates, seed)` accepts any nonzero state
dimension, explicit initial hypotheses, and deterministic seeding. Set a
motion callback and log-likelihood callback before predicting or updating.

```cpp
SequentialMonteCarlo filter(initialStates, 42);
filter.setMotionModel(motionModel);
filter.setLogLikelihoodModel(logLikelihood);
filter.setResamplingThreshold(0.5);
filter.predict(controlVector, dt);
filter.update(measurementVector);
```

The motion callback receives state, control, time interval, and the persistent
random generator. The likelihood callback receives state and measurement,
and returns log likelihood. Log weights are normalized after subtracting
their maximum, preventing ordinary exponential underflow. Systematic
resampling is controlled by an effective-sample-size ratio; zero disables it.
If every particle is assigned negative-infinity likelihood, weights revert
to uniform without inventing new states. Invalid dimensions/nonfinite values
are rejected. The old particle-count constructor retains its three-state
random-walk and leading-state measurement defaults for compatibility.

## Reward-Optional Learning Interface

`robot_learning.h` defines `Observation`, `ActionSpace`, `Environment`,
`Policy`, `Transition`, `RewardFunction`, `TransitionSink`, and `EpisodeRunner`.
A controller can supply the policy; the environment supplies actual dynamics
and observations. A different policy can use the same bounded action space.

```cpp
learning::EpisodeRunner runner;
auto trajectory = runner.run(environment, controllerPolicy, 1000);
auto scored = runner.run(environment, controllerPolicy, 1000, rewardFunction);
```

Rewards remain absent unless a callback is supplied. Episode limits are marked
as truncations; environment completion/contact may terminate an episode.
The sink receives transitions for later replay storage or world-model work.
No optimization/training or reward design is implied by collecting episodes.

### Exit Quickness Reward

`exitQuicknessReward(ExitRewardConfig)` supplies a task reward without changing
the generic episode runner. Observations include an environment-verified
`exitReached` flag. The default charges 1 per simulated second and awards 100
once on a collision-free exit. A successful episode returns `100 - seconds`.

Failed termination or truncation charges 100 plus the cost of all remaining
budget seconds. Early collisions therefore cannot earn a better failure
score simply by ending early. Rates, bonuses, and budget are configurable;
Robot Lab derives the budget from the episode step limit. Reward compares
completion time, not raw wheel speed or frame rate. The reward callback
does not itself train a policy.

## Trainable Controller Supervisor

`ControllerLearner` wraps the existing `ML/multi_arm_bandit` UCB agent.
`UCBAgent::select_arm()` and `observe_reward()` now support externally measured
rewards, while `run_simulation()` remains compatible with existing callers.
The robot adapter normalizes real completed exit returns to the fixed task's
reward range and learns a speed profile from six candidate limits. It is a
macro-action bandit policy, not a learned continuous steering controller.

The learner supports exploration, best-observed-profile evaluation, statistics,
and versioned checkpoint save/load. Loading validates profile definitions,
counts, finite rewards, and task budget; reconstructing the history preserves
selection behavior. Robot Lab supplies actual episodes and never uses the
bandit arms' synthetic Bernoulli pulls for training.

The UCB and navigation sources are linked statically into this adapter so the
native app does not introduce another unsigned runtime DLL. Use
`robot_simulator --train-rl` to train and `--episode --policy rl` to evaluate.
Model state is persisted; lidar, pose estimation, and collision-safe steering
remain the existing EKF/particle and DWA algorithms.

## Verification

Focused CTest cases cover odometry, EKF/particle correction, heading wrap,
clear/blocked/invalid scans, bounded actions, optional rewards, and truncation.
Tracker regression cases also cover custom state dimension, injected control,
log-weight underflow, validation, and deterministic noise. Robot Lab's
`--diagnostic` exercises both localizers and complete controller episodes.