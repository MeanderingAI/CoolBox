# CoolBox Python Bindings

Python bindings for the CoolBox C++ machine learning toolkit.

## Install

- `pip install git+https://github.com/MeanderingAI/CoolBox.git`
- `pip install git+https://github.com/MeanderingAI/CoolBox.git#subdirectory=_deliverables/libraries/bindings/python_bindings`
- `pip install .` from `_deliverables/libraries/bindings/python_bindings`

If you are already at the repository root, `pip install .` also works there now.

After installation, both import styles are supported:

- `import ml_toolbox`
- `import ml_core`

## Included modules

- `decision_tree`
- `svm`
- `bayesian_network`
- `hmm`
- `glm`
- `multi_arm_bandit`
- `tracker`
- `dimensionality_reduction`
- `deep_learning`
- `computer_vision`
- `time_series`
- `nlp`
- `distributed`
- `rest_api`
- `battery_simulator`
- `graphics`

## Battery Simulator Usage

After install/build, you can use the battery simulation bindings from Python:

- `from ml_toolbox import battery_simulator as bs`

Example:

```python
from ml_toolbox import battery_simulator as bs

cell = bs.Cell("cell-1", bs.Chemistry.LITHIUM_ION, 2.5, 0.05, 1.0)
pack = bs.BatteryPack("pack", 4, 2, cell)

energy_wh = pack.discharge(10.0, 30.0)
print(pack.pack_voltage(), energy_wh)
```

Higher-level script-style workflow:

```python
from ml_toolbox import battery_simulator as bs

sim = bs.PackSimulator.from_cell_config(
	label="demo_pack",
	series=4,
	parallel=2,
	chemistry=bs.Chemistry.LITHIUM_ION,
	initial_soc=0.65,
)

history = sim.run_script(
	[
		{"command": "discharge", "duration_s": 120, "current_a": 8.0, "step_s": 5.0},
		{"command": "rest", "duration_s": 30, "step_s": 5.0},
		{
			"command": "optimization",
			"duration_s": 240,
			"step_s": 5.0,
			"current_limit_a": 12.0,
			"target_soc": 0.9,
		},
	]
)

last = history[-1]
print(last.action, last.average_soc, last.terminal_voltage_v)
```

Run a text script file and export CSV:

`sim_script.txt`

```txt
# command key=value key=value ...
discharge duration_s=120 current_a=8.0 step_s=5
rest duration_s=30 step_s=5
optimization duration_s=240 step_s=5 current_limit_a=12.0 target_soc=0.9
```

```python
from ml_toolbox import battery_simulator as bs

sim = bs.PackSimulator.from_cell_config("demo_pack", 4, 2)
history = sim.run_script_file("sim_script.txt")
csv_path = sim.export_history_csv(history, "outputs/sim_history.csv")
print(csv_path)

# or one call:
csv_path = sim.run_script_file_to_csv("sim_script.txt", "outputs/sim_history.csv")
```

Runnable demo in repo:

- Script file: `_deliverables/libraries/bindings/python_bindings/examples/battery_simulator_script.txt`
- Demo runner: `_deliverables/libraries/bindings/python_bindings/examples/battery_simulator_demo.py`
- Graphics demo runner: `_deliverables/libraries/bindings/python_bindings/examples/battery_simulator_graphics_demo.py`
- Split graphics demo runner: `_deliverables/libraries/bindings/python_bindings/examples/battery_simulator_graphics_split_demo.py`

Run it from the bindings directory:

```bash
python examples/battery_simulator_demo.py
python examples/battery_simulator_graphics_demo.py
python examples/battery_simulator_graphics_split_demo.py
```

The graphics demo uses the pip-installed `ml_toolbox` package modules only and
produces both:

- CSV output (`examples/outputs/battery_sim_history.csv`)
- PNG plot (`examples/outputs/battery_sim_history.png`)

The split graphics demo produces two separate images:

- SoC plot (`examples/outputs/battery_sim_soc.png`)
- Terminal voltage plot (`examples/outputs/battery_sim_terminal_voltage.png`)

## Build

From the repository root:

- `make build_python_bindings`

Or directly:

- `cd _deliverables/libraries/bindings/python_bindings && python -m build`

## Native Library Resolution

- `setup.py` first tries to link against prebuilt CoolBox native libraries discovered under `COOLBOX_LIB_DIR`.
- Library discovery is recursive, so staged CMake outputs do not need to live directly in the top-level `build` directory.
- If `charts` or `wave_generator_utils` cannot be resolved from the staged build tree, the build falls back to compiling the vendored source copies shipped with the Python bindings package.
- You can override the search behavior with:
	- `COOLBOX_LIB_DIR` to point at a specific build tree root
	- `COOLBOX_LIBS` to control which native libraries are requested

See `INSTALL.md` for more details.