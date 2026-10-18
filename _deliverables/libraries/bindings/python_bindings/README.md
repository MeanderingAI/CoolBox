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
- `matrix` (dense matrices, vectors, CPU optimizations, Eigen, and optional GPU backends)
- `opt`, `math`, and `finance` (optimizers, heat solvers, pricing, Monte Carlo,
  and distributed model averaging)

Matrix API example:

```python
from ml_toolbox import matrix

left = matrix.DenseMatrix.from_rows([[1.0, 2.0], [3.0, 4.0]])
right = matrix.DenseMatrix.identity(2)
product = left @ right
print(product.data)
```

The matrix bindings provide dense matrix construction, indexing, addition,
subtraction, multiplication, transpose, norms, row-wise vector addition, and
dense-vector addition/dot products. Select `matrix.MatrixBackend.EIGEN` with
`matrix.set_backend(...)` to run matrix operations through MATRIX's Eigen-backed
types; `matrix.MatrixBackend.CPU` selects the existing optimized CPU kernels.
The selection is process-wide. Eigen headers are required when building the
native extension and can be located with `EIGEN3_INCLUDE_DIR` or
`EIGEN_INCLUDE_DIR`. CPU optimization strategies apply when the CPU backend
is selected. Backends are process-wide; `get_backend()` returns the requested
backend and `get_active_backend()` reports the one resolved for the most recent
operation. `AUTO` selects an available GPU backend and otherwise CPU. CUDA is
available only when the native extension is compiled with nvcc and a CUDA
device/runtime is present. Metal requires an Objective-C++ build on macOS with
a usable Metal device, with `metal_backend.mm` and Metal/Foundation frameworks
linked. These GPU choices currently accelerate matrix addition,
multiplication, and transpose. Norms and scalar access remain host-side;
unsupported vector arithmetic, matrix subtraction, and row-wise operations
report errors instead of silently claiming GPU execution. OpenCL is listed but
unavailable until its kernels are implemented; check `backend_available(...)`
before selecting it.

## OPT, MATH, and FINANCE

The `opt` submodule exposes all four optimizers, linear-model OWA and its
leave-one-model-out cross-validation helper, direct weighted tensor averaging,
and nonlinear optimal averaging for deep-learning model layers. The
`nonlinear_optimal_weighted_average_layers` callback receives the current
candidate merged layers as a list of `deep_learning.Tensor` objects and must
return a scalar validation loss. `OwaConfig` controls the optimization.

The `math` submodule provides deterministic and seeded stochastic heat
equation solvers. The `finance` submodule includes pricing formulas,
Brownian/GBM paths, built-in Monte Carlo contracts and Greeks, plus
`monte_carlo_estimate_custom_payoff` and
`monte_carlo_greeks_custom_payoff_fd` for user-defined payoffs. Custom payoff
callbacks receive the complete simulated price path, including the initial
spot, and run synchronously; callback exceptions abort the calculation and
propagate to Python.

Native Go bindings are available in
[`../go_bindings`](../go_bindings/README.md). They currently include the
metadata client, UUID generation, GLM linear regression, K-Means clustering,
KNN search, decision trees, multi-armed bandit arms, NLP text preprocessing,
univariate time-series utilities and forecasting models, deep-learning tensors,
layers, losses, optimizers and networks, the HELiX learner, and line/bar/scatter
chart rendering with image export. Additional Python modules are not yet
ported; see the Go bindings README for detailed coverage and limitations.

The `graphics.plot_2d` and `graphics.plot_3d` helpers accept `show_window=True`
to open an interactive browser window. Scroll to zoom, drag to pan, and use the
Save PNG button (or S) to export the original plot. The viewer runs locally
from a temporary page; plot data is not uploaded.

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