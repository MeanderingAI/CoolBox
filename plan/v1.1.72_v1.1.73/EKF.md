# Extended Kalman Filter (EKF) Migration (v1.1.73)

- Migrated EKF header and source from Eigen to mytrix types (`matrix::DenseMatrix`, `std::vector<double>`).
- Updated all math and logic to use new types and accessors.
- Validated error-free build for these files.
- No README in module directory; documentation is tracked here per user request.

Date: 2026-04-22

---

## 2026-04-22: Fixed FullApplicationWindow Include Path in body_generator

- Fixed build error in `_Product/body_generator/src/ui_main.cpp` due to incorrect include path for `full_application_window.hpp`.
- Updated include from `<graphics/full_application_window/headers/full_application_window.hpp>` to `<full_application_window.hpp>` to match CMake include directories.
- Confirmed that the header is available at `_libraries/packages/GRAPHICS/full_application_window/headers/full_application_window.hpp` and CMake target includes this directory.
- This resolves linker and build errors for the body_generator UI target.
