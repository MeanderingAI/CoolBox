# CoolBox Rust Bindings

Rust bindings for the APIs covered by the CoolBox Go package. Native C++ APIs
reuse the Go package's C ABI bridge functions; K-Means and KNN are implemented
in Rust. Python is not required at runtime.

## Coverage

- Client metadata, UUID generation, and generalized linear regression
- K-Means clustering, KNN search, decision-tree classification/regression, and
  Bernoulli bandit arms
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

This provides Rust coverage of the Go binding surface, not complete parity with
all Python APIs. Advanced Python-only functionality (for example transformers,
attention, SVMs, PCA/SVD/UMAP, computer vision, and the additional chart types)
is outside this crate's current scope.

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

The helpers default to an 800x600 canvas and X/Y/Z axis labels. `show_window`
is opt-in but currently returns an explicit unsupported-build error in the
Rust bridge; the native C++ API has the interactive viewer.

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
