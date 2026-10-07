# Robot Lab

A native 3D, differential-drive mobile robot simulator for CoolBox. Raylib
renders the scene; Bullet handles rigid-body contacts and lidar raycasts.
The robot is constrained to a level floor. Wheels are visualized; this is
not a suspension, motor-electrical, or traction model.

## Build

Enable the optional `BUILD_ROBOT_SIMULATOR` CMake option, then build the
`robot_simulator` target in Release configuration. The option defaults to
off so normal CoolBox configuration does not fetch these dependencies.
Raylib 5.5 and Bullet 3.25 are fetched on the first enabled configuration.

On Windows the executable is at
`build/_deliverables/apps/robot_simulator/Release/robot_simulator.exe`.

On Linux, install the required compiler and graphics development packages:

```bash
sudo apt-get install build-essential cmake libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

Then select **Run robot_simulator** in VS Code, or run:

```bash
./_deliverables/apps/robot_simulator/run_linux.sh
```

The script checks prerequisites, configures the optional simulator target,
builds it in Release mode, and starts it. Extra command-line arguments are
passed to the simulator.

## Controls

- WASD or arrow keys: forward, reverse, and differential steering.
- The on-screen directional controls also support mouse-held driving.
- Space: pause/resume. R: reset pose, trail, and telemetry.
- Tab: switch between manual driving and autonomous waypoint patrol.
- C: switch between follow and orbit cameras.
- Right-drag in orbit mode: rotate the camera. Scroll: zoom.
- Speed slider: maximum translational command from 0.5 to 4 m/s.
- Lidar checkbox: show/hide the 11-ray, 160-degree, 8 m sensor fan.
- Capture: save the current window to `robot-lab.png` in the working directory.

Patrol uses `cool_car/CONTROL`'s Dynamic Window local planner, not the original
heading-only controller. It samples bounded speed/turn commands, rolls out
trajectories, and checks footprint clearance and braking distance against
the known map and lidar hits. It does not guarantee a globally optimal route
or escape from every obstacle arrangement.

Choose EKF or PARTICLES in the localization controls. Both reuse existing
tracker algorithms and fuse wheel odometry, lidar distances, and a simulated
orientation/forward-vector sensor. Patrol uses the estimated pose, not
Bullet's true position. The yellow marker is the estimated pose; particle
mode also shows its hypothesis cloud. The blue line is the chosen trajectory,
and green sensor directions have clearance under the current braking check.
ERROR compares localization with simulation ground truth for evaluation only;
SIGMA is estimated planar uncertainty.

Simulator odometry uses the motion Bullet actually resolves each physics step,
so contact with a wall does not advance the estimated pose when the chassis is
blocked.

Localization assumes a known obstacle map and approximate starting pose.
This is not SLAM or global relocalization. Manual driving remains unrestricted
by the planner, with Bullet enforcing physical collisions.

## Diagnostics And Captures

`robot_simulator.exe --diagnostic` checks drive, steering, lidar, collisions,
reset, both localizers, blocked-scan stopping, an EKF-controlled patrol, and a
particle-controlled physical exit, including time-based reward checks,
without creating a window.

`--capture PATH --frames 180` runs a bounded visual session in patrol mode
and saves a PNG before exiting. `--width` and `--height` set the window
size; the minimum supported size is 1000 x 700.
`--localizer particle` selects particle localization for visual captures.
`--mission exit` selects the exit task; `--mission patrol` keeps the looped
patrol. The EXIT control resets the robot and starts a timed run through the
north-wall gate into a clearing beside a broad-canopied fig tree. The camera
widens near the clearing, and the run pauses on exit, collision, or its 120 s
GUI time limit. Reward uses simulated time, excluding pauses.

## Learning And RL

`robot_simulator.exe --episode --steps 1000 --localizer particle` runs a
window-free episode using the existing DWA controller as the baseline policy.
The localizer may also be `ekf`. Episodes end on contact, physical exit
crossing, or the step limit; each environment step advances up to ten 120 Hz
physics ticks. `--speed-limit 1.2` provides a slower baseline for comparison.

The `robot_learning.h` interface provides observations, bounded actions,
environment reset/step, transitions, policy callbacks, an optional reward
callback, and a transition sink for later storage/training. The generic runner
still leaves reward absent without a callback, but Robot Lab episodes now
use `exitQuicknessReward` by default. Baseline episodes use DWA. The enabled
RL supervisor learns which speed profile to use; DWA still handles all
steering, lidar/map clearance, and braking checks.

### Train And Use RL

Run from the repository root:

```powershell
$robot = ".\build\_deliverables\apps\robot_simulator\Release\robot_simulator.exe"
& $robot --train-rl --episodes 18 --model build/robot-exit-policy.txt
& $robot --episode --policy rl --model build/robot-exit-policy.txt
& $robot --policy rl --model build/robot-exit-policy.txt
```

Training reuses the existing `UCBAgent`, exploring six controller speed limits:
0.8, 1.2, 1.8, 2.4, 3.2, and 4.0 m/s. Each trial runs actual Bullet simulation
and updates the selected profile from its real exit return. This is episodic
bandit RL/controller selection, not a neural policy or end-to-end learned
steering. The current arena, initial pose, reward, and DWA planner define the
task; generalization to other maps has not been demonstrated.

Checkpoints store versioned profile definitions, time budget, exploration
coefficient, and episode-return history. Existing checkpoints resume training
when passed to `--train-rl`; `--steps` must match the checkpoint budget.
Evaluation selects the highest observed mean return without exploration or
updates. Training saves after each episode, checks loading, and reports a
learned-policy evaluation against the 2.4 m/s baseline.

In the window, **RL** starts the best observed controller. **Train Episode**
explores one profile, updates and saves on exit/collision/timeout, then pauses.
Manual mode, reset, and Tab interruption cancel a partial training trial rather
than recording it as a completed trial. The speed slider is replaced by the
selected learned profile and training statistics in RL mode.

Verified on the fixed particle-localized arena: 18 training trials completed
without contact. Reloaded evaluation selected 4.0 m/s and exited in 10.917 s
with return 89.083, compared with the baseline's 12.225 s and return 87.775.
These are deterministic simulator checks, not a broad performance benchmark.

### Exit Reward

- Every transition costs 1 reward unit per elapsed simulated second.
- Completing a full four-marker lap awards 20 points once per completed lap.
- Reaching the exit without collision awards 100 once: successful return
	includes the exit, circle, and lap bonuses minus completion time, so faster
	runs with completed laps score higher.
- Collision or timeout costs 100 plus the unused time budget. Thus all failed
	episodes score `-100 - time_budget_seconds`, preventing early crashes from
	avoiding time penalties. The CLI budget is `steps * 10 / 120` seconds.
- `ExitRewardConfig` exposes time cost, exit bonus, failure penalty, and time
	budget plus circle and lap bonuses for later tuning. No distance/progress
	shaping is added.
- Exit completion is verified against Bullet's actual position after the
	entire chassis clears the gate, not against the localization estimate.

Windows code-integrity policies may still require an approved signature
on this executable. The simulator does not change Windows security policy.