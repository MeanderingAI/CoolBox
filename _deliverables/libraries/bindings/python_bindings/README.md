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
- `graphics` (including projected 3D line and scatter plots)
- `synthetic_data`
- `timer`

## Train a GAN

`ml_toolbox.GAN` provides a small fully connected GAN trainer built from the
CoolBox deep-learning layers. Pass real batches as two-dimensional
`ml_toolbox.ml_core.deep_learning.Tensor` values scaled to `[-1, 1]`.
`train_batch` returns the discriminator and generator binary cross-entropy
losses; `sample(count)` generates samples.

See `examples/gan_training_example.py` for a runnable toy-data example that
plots both training losses with CoolBox's built-in graphics module and saves
the chart to `gan_losses.png`.

## Build

From the repository root:

- `make build_python_bindings`

Or directly:

- `cd _deliverables/libraries/bindings/python_bindings && python -m build`

CMake skips pybind11 and Python bindings when targeting Windows ARM64 from a
non-ARM64 host, including Visual Studio `-A ARM64` builds that do not set
`CMAKE_CROSSCOMPILING`. Native ARM64 hosts retain Python bindings support and
require a matching ARM64 Python installation.

## Native Library Resolution

- `setup.py` first tries to link against prebuilt CoolBox native libraries discovered under `COOLBOX_LIB_DIR`.
- Library discovery is recursive, so staged CMake outputs do not need to live directly in the top-level `build` directory.
- If `charts` or `wave_generator_utils` cannot be resolved from the staged build tree, the build falls back to compiling the vendored source copies shipped with the Python bindings package.
- You can override the search behavior with:
	- `COOLBOX_LIB_DIR` to point at a specific build tree root
	- `COOLBOX_LIBS` to control which native libraries are requested

See `INSTALL.md` for more details.