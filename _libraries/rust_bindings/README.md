# CoolBox Rust Bindings

This crate provides a small Rust wrapper around selected CoolBox C++ components.

## Included bindings

- Linear regression backed by the CoolBox generalized linear model implementation

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

## Build

```bash
cmake -S ../c_bindings -B ../c_bindings/build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build ../c_bindings/build --config Release
cargo build --release
cargo test
```
