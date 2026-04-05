# CoolBox Python Bindings

Python bindings for the CoolBox C++ machine learning toolkit.

## Install

- `pip install git+https://github.com/MeanderingAI/CoolBox.git`
- `pip install git+https://github.com/MeanderingAI/CoolBox.git#subdirectory=_libraries/python_bindings`
- `pip install .` from `_libraries/python_bindings`

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

## Build

From the repository root:

- `make build_python_bindings`

Or directly:

- `cd _libraries/python_bindings && python -m build`

## Native Library Resolution

- `setup.py` first tries to link against prebuilt CoolBox native libraries discovered under `COOLBOX_LIB_DIR`.
- Library discovery is recursive, so staged CMake outputs do not need to live directly in the top-level `build` directory.
- If `charts` or `wave_generator_utils` cannot be resolved from the staged build tree, the build falls back to compiling the vendored source copies shipped with the Python bindings package.
- You can override the search behavior with:
	- `COOLBOX_LIB_DIR` to point at a specific build tree root
	- `COOLBOX_LIBS` to control which native libraries are requested

See `INSTALL.md` for more details.