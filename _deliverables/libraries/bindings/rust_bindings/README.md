# CoolBox Rust Bindings

Rust bindings for the APIs covered by the CoolBox Go package. Native C++ APIs
reuse the Go package's C ABI bridge functions; K-Means and KNN are implemented
in Rust. Python is not required at runtime.

## Coverage

- Client metadata, UUID generation, and generalized linear regression
- K-Means clustering, KNN search, decision-tree and boosted-tree
  classification/regression, and Bernoulli bandit arms
- Text processing, time-series statistics/transforms, and AR/ARMA/ARIMA and
  moving-average/exponential-smoothing forecasting
- Tensors, arithmetic, deep-learning layers, losses, optimizers, neural
  networks, and the HELiX learner
- Line/bar/scatter plotting, single-series 2D/3D helpers with default axis
  labels, RGBA pixel access, and BMP/PNG/JPEG export
- The data structures available in the Go bindings, including linked lists,
  queues, heaps, maps, ordered trees, filters, Van Emde Boas trees, arrays,
  B+ trees, radix trees, LSM and Maple trees, and cover trees
- Dense matrices and vectors with matrix arithmetic, dot/add, transpose,
  broadcasting, norms, selectable CPU optimizations, Eigen, and optional GPU backends
- Genetic search, simulated annealing, hill climbing, quantum-control Bayesian
  optimization, 1D–3D heat/PDE and seeded SPDE solvers, finance pricing and
  Monte Carlo estimates/Greeks, Ito utilities, and distributed linear-model
  parameter averaging, fast OWA cross-validation, nonlinear tensor-layer OWA,
  and custom-payoff Monte Carlo estimates/Greeks

This provides Rust coverage of the Go binding surface, not complete parity with
all Python APIs. Advanced Python-only functionality (for example transformers,
attention, SVMs, PCA/SVD/UMAP, computer vision, and the additional chart types)
is outside this crate's current scope.

The service API also includes Prisma-schema parsing, SQL database connections,
migrations, model repositories, and a route-registration HTTP server. Database
rows are returned as JSON strings; `HttpServer::start` blocks until `stop` is
called. Enable PostgreSQL or MySQL when building the native C bindings with
`COOLBOX_SQL_ENABLE_POSTGRESQL=ON` or `COOLBOX_SQL_ENABLE_MYSQL=ON`.
For language-neutral routing, register named handlers and call
`HttpServer::load_routes` with the shared `.routes` manifest.

## Example

```rust
use coolbox_rs::{FitMethod, LinearRegression};

fn main() -> Result<(), String> {
    let x = vec![vec![1.0], vec![2.0], vec![3.0], vec![4.0]];
    let y = vec![3.0, 5.0, 7.0, 9.0];

    let model = LinearRegression::fit(&x, &y, FitMethod::ClosedForm, 500, 0.01)?;
    let predictions = model.predict(&x)?;

    println!("weights = {:?}", model.weights());
    println!("intercept = {}", model.intercept());
    println!("predictions = {:?}", predictions);
    Ok(())
}
```

Boosted-tree regression and classification:

```rust
use coolbox_rs::{BoostTree, BoostTreeClassifier, BoostTreeParameters};

fn main() -> Result<(), String> {
    let params = BoostTreeParameters::default();
    let features = vec![vec![0.0], vec![1.0], vec![2.0], vec![3.0]];

    let mut regressor = BoostTree::new(params)?;
    regressor.fit(&features, &[0.0, 1.0, 2.0, 3.0])?;
    println!("regression predictions = {:?}", regressor.predict_batch(&features)?);

    let mut classifier = BoostTreeClassifier::new(params)?;
    classifier.fit(&features, &[-3, -3, 8, 8])?;
    println!("classes = {:?}", classifier.classes()?);
    println!("prediction = {}", classifier.predict(&[3.0])?);
    Ok(())
}
```

Tensor example:

```rust
use coolbox_rs::Tensor;

fn main() -> Result<(), String> {
    let left = Tensor::new(&[2, 2], Some(&[1.0, 2.0, 3.0, 4.0]))?;
    let right = Tensor::new(&[2, 2], Some(&[5.0, 6.0, 7.0, 8.0]))?;
    let product = left.matmul(&right)?;
    println!("{:?}", product.data()?);
    Ok(())
}
```

OPT/MATH/FINANCE example:

```rust
use coolbox_rs::{
    evaluate_finance_formula, solve_heat_equation, FinanceFormula, HeatGrid,
    SearchOptimizer, OptimizerType,
};

fn main() -> Result<(), String> {
    let price = evaluate_finance_formula(
        FinanceFormula::BlackScholesCallPrice,
        &[100.0, 100.0, 0.05, 0.2, 1.0],
    )?;
    println!("call price = {price}");

    let result = solve_heat_equation(
        &HeatGrid {
            values: vec![0.0, 1.0, 0.0],
            nx: 3,
            ny: 1,
            nz: 1,
        },
        1,
        1.0,
        0.0,
        [1.0, 1.0, 1.0],
        0.1,
        1,
        42,
        false,
    )?;
    println!("heat solution = {:?}", result.values);

    let mut optimizer = SearchOptimizer::new(
        OptimizerType::HillClimbing,
        &[1.0, 40.0, 20.0, 1.0, 0.25, -10.0, 10.0, 2026.0],
    )?;
    let solution = optimizer.optimize(&mut |point| point[0] * point[0], &[4.0])?;
    println!("minimum near = {solution:?}");
    Ok(())
}
```

`FinanceFormula` covers the native Black-Scholes, digital, Asian, and barrier
formulas. Monte Carlo contracts use common-random-number shock paths. Optimizer
config slices override native defaults in each C++ config's field order; an
empty slice uses those defaults. Heat-grid values are row-major, with 3D data
laid out as `[x][y][z]`.

`fast_owa_cross_validation_linear_regression` accepts one validation fold per
local model. Each fold's feature rows must match the model parameter width.
`nonlinear_optimal_weighted_average_layers` operates on `Vec<Vec<Tensor>>` and
passes snapshots of candidate merged layers to its validation callback. The
returned merged tensors are caller-owned.

`estimate_monte_carlo_custom_payoff` and its Greeks variant invoke a payoff
closure with each complete simulated path, including the initial spot.
Callbacks run synchronously as part of the native calculation and can return
an error to abort it.

Convenience plotting helpers:

```rust
use coolbox_rs::{plot_2d, Plot2DOptions};

fn main() -> Result<(), String> {
    let canvas = plot_2d(
        &[0.0, 1.0, 2.0],
        &[1.0, 3.0, 2.0],
        Plot2DOptions {
            title: "Measurements".to_string(),
            ..Plot2DOptions::default()
        },
    )?;
    canvas.save("measurements.png", coolbox_rs::ImageFormat::Png, 90)?;
    Ok(())
}
```

The helpers default to an 800x600 canvas and X/Y/Z axis labels. Set
`show_window: true` to open an interactive browser window with wheel zoom,
drag-to-pan, and a Save PNG control. The viewer renders locally from a
temporary page and does not upload plot data.

## Build

Building requires a C++17 compiler for the same target architecture as Rust and
a built CoolBox C API library. On Windows, the selected Rust MSVC toolchain also
requires the Visual C++ build tools, including `link.exe`.

```bash
cmake -S ../c_bindings -B ../c_bindings/build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build ../c_bindings/build --config Release
cargo build --release
cargo test
```

The C API library and Rust crate must be built with compatible native
toolchains/architectures. If the C API is built outside the default
`../c_bindings/build` directory, set `COOLBOX_C_BINDINGS_BUILD_DIR` to that
directory. Set `COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR` when the library is in a
configuration subdirectory such as `Release`. Eigen headers are required;
`build.rs` checks `external/eigen/eigen-3.4.0` and also accepts
`EIGEN3_INCLUDE_DIR` or `EIGEN_INCLUDE_DIR`. `MatrixBackend::Eigen.set()` uses
the MATRIX Eigen-backed operations, `MatrixBackend::Cpu.set()` selects the
optimized CPU path, and `MatrixBackend::Auto.set()` selects an available GPU
or CPU otherwise. Selection is process-wide; `MatrixBackend::current()` reads
the request, `MatrixBackend::active()` reports the most recently resolved
backend, and `is_available()` checks build and runtime support. CUDA requires
compiling the native bridge with nvcc and a usable CUDA device/runtime. Metal
requires an Objective-C++ build on macOS and a usable Metal device. These GPU
backends require compiling/linking `metal_backend.mm` with the Metal and
Foundation frameworks. They accelerate matrix addition, multiplication, and
transpose. Norms and
scalar access remain host-side; unsupported vector arithmetic, matrix
subtraction, and row-wise operations return errors instead of silently
claiming GPU execution. OpenCL remains unavailable until its kernels are
implemented.

`coolbox_rs::set_cpu_threads(8)` sets the process-wide CPU/Eigen multiplication
thread budget; `1` forces serial execution and `0` selects hardware concurrency.
`coolbox_rs::cpu_threads()` returns the resolved budget. Small products stay
serial to avoid scheduling overhead. Do not mutate matrices during concurrent
reads.

For optional OpenBLAS multiplication, build with `MYTRIX_ENABLE_OPENBLAS=1`
and installed OpenBLAS development files. `OPENBLAS_INCLUDE_DIR` and
`OPENBLAS_LIBRARY_DIR` support nonstandard installations; ensure the runtime
loader can also locate the library. Check `MatrixBackend::OpenBlas.is_available()`
before `MatrixBackend::OpenBlas.set()`. The CPU thread budget controls OpenBLAS's
own scheduler. Other operations use CPU kernels and report CPU as active.
CoolBox serializes OpenBLAS calls and restores its previous global thread setting;
external code must not change that setting concurrently.
