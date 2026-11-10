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

## CPU multiplication and threads

The binding CPU backend uses cache-blocked, row-major double-precision GEMM with
4-by-8 register tiles (NEON on AArch64, runtime-dispatched AVX2/FMA on GCC/Clang
x86, and a portable fallback). It no longer gathers every RHS column and
recomputes independent dot products. The Eigen binding backend maps existing
row-major buffers directly rather than copying and converting every operand.

Both paths partition sufficiently large products into disjoint output rows.
The header-only C++ Eigen wrapper also parallelizes large products, partitioning
output columns for its column-major storage. No OpenMP or BLAS dependency is
required. The `mytrix` CMake target propagates the platform thread library.
Small products stay serial to avoid thread startup overhead; `cpu_threads()`
reports the budget, not the number necessarily used by every operation.
The scheduler requires at least 64 Mi multiply-add iterations per worker,
as well as approximately 32 output rows (columns in the C++ Eigen wrapper).
For square products and an eight-thread budget this keeps 256-by-256 serial,
uses two workers at 512-by-512, and allows eight at 1024-by-1024. This
conservative policy avoids launching all workers at the first parallel size;
the crossover still depends on hardware and system load.

- C++: `mytrix::set_cpu_threads(8)` / `mytrix::cpu_threads()`.
- Shared native bridge: `coolbox::matrix::set_cpu_threads(8)`.
- Python: `ml_toolbox.matrix.set_cpu_threads(8)` / `cpu_threads()`.
- Go: `SetMatrixCPUThreads(8)` / `MatrixCPUThreads()`.
- Rust: `coolbox_rs::set_cpu_threads(8)` / `cpu_threads()`.

The process-wide budget defaults to `0` (hardware concurrency); `1` forces serial
execution. Set it before running concurrent workloads to avoid oversubscription.
Matrices must not be modified while another thread reads them. Floating-point
rounding may differ from NumPy/Eigen because of FMA and accumulation order;
compare results with numerical tolerances rather than exact equality.

For reproducible float64 comparisons against NumPy, use the Python binding's
`benchmarks/benchmark_matrix_numpy.py`. Compare matching thread budgets and keep
construction/conversion outside timed multiplication. Speed relative to tuned
OpenBLAS depends on shape, size, hardware and scheduling; there is no guarantee
of outperforming NumPy at every size.

## Optional OpenBLAS backend

Set `MYTRIX_ENABLE_OPENBLAS=ON` when configuring CMake to enable double-precision
matrix multiplication through OpenBLAS's `cblas_dgemm`. This is opt-in: CPU,
Eigen, and Auto keep their existing behavior. Missing OpenBLAS headers or
libraries cause a configuration error when the option is enabled.

CMake finds `cblas.h` and `libopenblas` (or `openblas.lib`). Supply `OpenBLAS_ROOT`
for a nonstandard installation, or set `MYTRIX_OPENBLAS_INCLUDE_DIR` and
`MYTRIX_OPENBLAS_LIBRARY` explicitly. The OpenBLAS runtime and its dependencies
must also be available to the OS loader. On Ubuntu, install `libopenblas-dev`;
on other platforms use a matching architecture/toolchain build of OpenBLAS.

- C++: `set_backend(ComputeBackend::OPENBLAS)`, then `DenseMatrix::multiply`.
  The legacy C++ `operator*` remains the CPU/Eigen expression path.
- Python: `matrix.set_backend(matrix.MatrixBackend.OPENBLAS)`.
- Go: `SetMatrixBackend(BackendOpenBLAS)` (build with `-tags coolbox_openblas`).
- Rust: `MatrixBackend::OpenBlas.set()` (build with `MYTRIX_ENABLE_OPENBLAS=1`).

Check availability before selection; requesting OpenBLAS in a build without it
raises an error. Non-multiplication operations continue using host CPU kernels
and report CPU as their active backend, while preserving the requested backend.
Operands are passed directly in their existing row/column-major layout; there
is no Python-list conversion or Eigen copy inside multiplication.

`set_cpu_threads` also controls OpenBLAS multiplication, which uses OpenBLAS's
own thread scheduler rather than CoolBox's workload-based worker cap.
OpenBLAS thread settings are process-global. CoolBox serializes its OpenBLAS
calls while setting the budget and restores the prior library setting afterward.
Do not change that same library's thread configuration concurrently from
external code. FMA/order differences require tolerance-based comparisons.

The backend uses your installed OpenBLAS, not NumPy's private bundled copy;
versions and CPU kernel selection can differ. Performance near NumPy is expected,
not guaranteed, and using the same BLAS is not a new multiplication algorithm.
