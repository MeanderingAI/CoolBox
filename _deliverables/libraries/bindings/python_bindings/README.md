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
- `data_structures` (linked lists, queues, heaps, maps, trees, filters, arrays,
  and metric trees)
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
The backend selection and CPU thread limit are process-wide. Control CPU
parallelism with `matrix.set_cpu_threads(n)`: `0` selects automatic sizing,
`1` forces serial execution, and larger values cap the CPU worker count.
`matrix.cpu_threads()` reports the resolved CPU thread budget; automatic mode
resolves to the available hardware concurrency. Small products may still run
serially. Matrix multiplication currently retains the Python GIL while the
native kernel runs, avoiding concurrent Python-side mutation of input matrices.
Eigen headers are required when building the
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

To compare native CPU multiplication against NumPy with correctness checks
outside the timed regions, run
`python benchmarks/benchmark_matrix_numpy.py --sizes 32 64 128 256 512 1024
--backends cpu eigen --threads auto 1 2 4`. The script prints JSON containing medians, speedups,
platform details, configured CoolBox thread limits, and NumPy BLAS thread-pool
metadata when `threadpoolctl` is installed. NumPy's own threading follows its
BLAS configuration and environment variables; the comparison does not assume
CoolBox is faster for every matrix size or machine.

### OpenBLAS (optional)

For `setup.py` / pip builds, enable `MYTRIX_ENABLE_OPENBLAS=1` in the build
environment and install OpenBLAS development headers/libraries first. Optional
`OPENBLAS_INCLUDE_DIR` and `OPENBLAS_LIBRARY_DIR` select a nonstandard installation.
CMake builds use `-DMYTRIX_ENABLE_OPENBLAS=ON` and optionally `-DOpenBLAS_ROOT=...`.
The OS must be able to load OpenBLAS and its runtime dependencies.

```python
from ml_toolbox import matrix

if not matrix.backend_available(matrix.MatrixBackend.OPENBLAS):
    raise RuntimeError("Rebuild CoolBox with OpenBLAS enabled")
matrix.set_backend(matrix.MatrixBackend.OPENBLAS)
matrix.set_cpu_threads(8)  # OpenBLAS manages its own worker scheduling
result = left @ right
```

OpenBLAS handles matrix multiplication directly on native buffers. Other
operations use host CPU kernels and report CPU as the active backend.
The default backend/Auto policy is unchanged. OpenBLAS thread settings are
global; CoolBox serializes its OpenBLAS calls and restores the previous setting.
External code must not change that library's thread configuration concurrently.
This uses the installed library, which may differ from NumPy's private OpenBLAS.
To compare, run the benchmark with `--backends cpu eigen openblas --threads 8`
and set `OPENBLAS_NUM_THREADS=8` before starting Python.

## Data structures

`ml_toolbox.data_structures` exposes the native trekker data structures for
int64 values and keys, float64 arrays and metric trees, and string-keyed radix
trees. The module includes singly/doubly and concurrent linked lists, folio
queues, both heap implementations, indexed priority queues, regular and
concurrent hash maps, binary-search/red-black/splay trees, B+ and Maple trees,
log-structured merge trees, Bloom and EGH probabilistic filters, Van Emde Boas
trees, rank-1 through rank-4 arrays, and standard/faster cover trees.

Filter membership checks are probabilistic and may return false positives.
Cover-tree queries return `(input_index, distance, item)` tuples. Native objects
support `close()` and Python context managers:

```python
from ml_toolbox import data_structures as ds

with ds.EGHFilter(expected_insertions=10_000, false_positive_rate=0.01) as seen:
    seen.insert(42)
    print(seen.contains(42))
```

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