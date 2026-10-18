# MATRIX Package

This package contains the matrix library, moved from DATASTRUCTURE/matrix.

The Python, Go, and Rust bindings provide a consistent dense-matrix and
dense-vector surface backed by the package's CPU kernels and Eigen types. It
includes row-major construction and access, zeros, ones, identity matrices,
element-wise addition/subtraction, matrix products, transpose, norms, row-wise
vector broadcasting, vector addition/dot products, CPU optimization, and a
process-wide CPU/Eigen/Auto/CUDA/Metal/OpenCL selector. The bindings share the
native bridge in `../../../bindings/go_bindings/matrix_bridge.cc` and use the
package's CPU optimization kernels, Eigen operations, or actual GPU kernels. `Auto` selects
a runtime-available GPU and otherwise CPU. CUDA requires the bridge to be
compiled with nvcc and a working CUDA device/runtime; Metal requires
Objective-C++ on macOS, `metal_backend.mm`, the Metal/Foundation frameworks,
and a working Metal device. GPU operations currently
accelerate matrix addition, multiplication, and transpose. Norms and scalar
access remain host-side; unsupported vector arithmetic, matrix subtraction,
and row-wise operations report an error instead of silently claiming GPU
execution. OpenCL is intentionally reported unavailable until its kernels and
runtime are implemented. Query backend availability before selecting an
optional backend.
