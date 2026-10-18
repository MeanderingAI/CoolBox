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
- Line/bar/scatter plotting, RGBA pixel access, and BMP/PNG/JPEG export
- The data structures available in the Go bindings, including linked lists,
  queues, heaps, maps, ordered trees, filters, Van Emde Boas trees, arrays,
  B+ trees, radix trees, LSM and Maple trees, and cover trees

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
configuration subdirectory such as `Release`.
