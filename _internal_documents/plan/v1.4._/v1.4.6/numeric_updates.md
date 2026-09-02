# v1.4.6 Numeric Updates

## Compute backend integration

Source commits:
- `c64b067` adding numeric libraries to build
- merged into `v1.4.5` via merge commit `20b2412`

### Backend selection model updates

- `ComputeBackend` expanded to include `BOOST` and `EIGEN` in addition to CPU/GPU backends.
- Added `OperationOptions` with:
	- `backend` (default `GPU_AUTO`)
	- `boost` toggle
- `BackendConfig` now tracks a boost-enabled flag (`set_boost_enabled` / `boost_enabled`) and can resolve with both requested backend and boost mode.
- Fallback logic now allows explicit `BOOST`/`EIGEN` selection and boost-preferred resolution for CPU/AUTO requests.

### Build system changes

- Updated `MATRIX/CMakeLists.txt` to include Metal shader/runtime sources when Metal is enabled on Apple:
	- `source/metal_backend.metal`
	- `source/metal_backend.mm`
- Added interface compile definition for shader source path:
	- `MYTRIX_METAL_SHADER_PATH=.../source/metal_backend.metal`

### Backend implementation additions

- `backend_config.h` simplified to include templated backend config implementation.
- `backend_config_templated.h` extended with backend enum, resolvers, config flags, and helper accessors for new numeric backends.
- `cuda_backend.h` expanded substantially for CUDA runtime integration paths.
- `matrix_dense.h` updated to route behavior through the expanded backend model.
- `metal_backend.h` updated and paired with new source implementations:
	- `source/metal_backend.mm`
	- `source/metal_backend.metal`

### Tests

- Added `tests/test_metal_backend.cpp` to cover Metal backend behavior.
