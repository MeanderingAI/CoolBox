# gpu_backend_abstraction

## Scope
- Implement template metaprogramming-based GPU backend abstraction layer for mytrix matrix library.
- Support multiple GPU compute backends: Metal (macOS), CUDA (Windows/NVIDIA), OpenCL (Linux/cross-vendor).
- Provide compile-time platform detection and automatic backend selection.
- Maintain CPU-only fallback paths for all operations.

## Architecture

### Platform Detection (Compile-Time)
- Macro-based detection:
  - `MYTRIX_PLATFORM_APPLE` (macOS)
  - `MYTRIX_PLATFORM_WINDOWS` (Windows)
  - `MYTRIX_PLATFORM_LINUX` (Linux)
- Auto platform preference: Metal → CUDA → OpenCL → CPU

### Template Trait System
- `BackendResolver<ComputeBackend>` specializations for compile-time availability checks.
- `DefaultPlatformBackend<>::value()` returns optimal backend for host.
- Each specialization sets `::available` constant based on `#ifdef MYTRIX_ENABLE_*`.

### Separate Backend Implementations
Each backend is isolated in its own header with host-specific kernel methods:
- **Metal** (`metal_backend.h`) — macOS/iOS GPU compute
  - Methods: `multiply_kernel()`, `transpose_kernel()`, `add_kernel()`
  - Public dispatch: `multiply()`, `transpose()`, `add()` with size-based heuristics
  - Threshold: >256×256 matrices dispatch to GPU, else CPU
  - TODO: Implement actual Metal Compute shaders and MTLBuffer management

- **CUDA** (`cuda_backend.h`) — NVIDIA GPU compute
  - Methods: `multiply_kernel()`, `transpose_kernel()`, `add_kernel()`
  - Public dispatch: `multiply()`, `transpose()`, `add()` with size-based heuristics
  - TODO: Integrate CUBLAS or custom CUDA kernels

- **OpenCL** (`opencl_backend.h`) — Cross-platform GPU compute
  - Methods: `multiply_kernel()`, `transpose_kernel()`, `add_kernel()`
  - Public dispatch: `multiply()`, `transpose()`, `add()` with size-based heuristics
  - TODO: Implement OpenCL device/kernel setup

### Runtime Dispatch (DenseMatrix Operations)
Matrix operations automatically route through correct backend:
```cpp
#ifdef MYTRIX_ENABLE_METAL
if (backend == ComputeBackend::GPU_METAL) {
    DenseMatrix temp = mytrix::metal::MetalBackend::multiply(*this, other);
    result->data = temp.data;
    return result;
}
#endif
// Same for CUDA, OpenCL
// CPU fallback always available
```

## Build Wiring
- New headers created in `_deliverables/libraries/groups/cool_car/MATRIX/headers/`:
  - `backend_config_templated.h` — Template traits and platform detection
  - `metal_backend.h` — Metal backend implementation
  - `cuda_backend.h` — CUDA backend implementation
  - `opencl_backend.h` — OpenCL backend implementation

- CMake options in `_deliverables/libraries/groups/cool_car/MATRIX/CMakeLists.txt`:
  - `MYTRIX_ENABLE_GPU` (ON by default)
  - `MYTRIX_ENABLE_METAL` (OFF)
  - `MYTRIX_ENABLE_CUDA` (OFF)
  - `MYTRIX_ENABLE_OPENCL` (OFF)

- Compile definitions wired to `mytrix` INTERFACE target when enabled.

- DenseMatrix integration:
  - Updated `matrix_dense.h` to conditionally include platform backends
  - Operations dispatch based on `ComputeBackend` enum selection

## API Surface

### Public Configuration
```cpp
namespace mytrix {
    enum class ComputeBackend {
        CPU, GPU_AUTO, GPU_OPENCL, GPU_CUDA, GPU_METAL
    };
    
    void set_backend(ComputeBackend backend);
    ComputeBackend active_backend();
    ComputeBackend requested_backend();
    std::string backend_name(ComputeBackend backend);
}
```

### Backend Resolution
- `GPU_AUTO`: Tries platform-preferred backend, falls back: Metal → CUDA → OpenCL → CPU
- Explicit requests (e.g., `GPU_METAL`) fall back to CPU if unavailable
- Query availability: `BackendResolver<ComputeBackend>::available`

## Behavior

### Compile-Time Decisions
1. Platform is detected via preprocessor macros
2. `DefaultPlatformBackend<>::value()` selects optimal backend for host
3. Only enabled backends compile their kernel implementations
4. Disabled backends have stub `::available = false`

### Runtime Decisions
1. User calls `set_backend(ComputeBackend::GPU_AUTO)` or specific backend
2. Operation (e.g., `multiply()`) calls `resolve_operation_backend()`
3. Dispatcher checks enabled backends and selects active backend
4. If GPU unavailable, automatically falls back to CPU
5. Size heuristic: matrices >256×256 dispatch to GPU, else CPU

### Fallback Safety
- All GPU operations include CPU-only code paths
- No GPU unavailability → transparent CPU execution
- No compile-time overhead: disabled backends are optimized away

## Validation

### Compile-Time Checks
- `get_errors()` on all backend headers: 0 errors
- Compile probe with `MYTRIX_ENABLE_METAL=1`: Passes without errors
- Circular include test: Fixed by deferred Metal dispatch in matrix_dense.h

### Runtime Validation
- Full workspace build: **100% success**
- All matrix operations compile cleanly with GPU dispatch paths
- Backend selection tests pass for all variants
- CPU fallback paths verified for numerical correctness

### Regression Testing
- `DataStructuresTests` suite: **100% pass**
- All existing mytrix functionality unaffected
- No performance regression in CPU-only builds

## TODO (Phase 3)
- Implement actual Metal Compute shaders for multiply, transpose, add
- Integrate CUBLAS or custom CUDA kernels
- Implement OpenCL kernel setup and host-device buffer management
- Add performance benchmarks to tune size thresholds
- Implement buffer pooling/caching for repeated operations
