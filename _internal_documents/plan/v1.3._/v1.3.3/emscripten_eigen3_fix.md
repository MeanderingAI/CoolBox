# Emscripten Bindings — Eigen3 Cross-Compilation Fix

## Problem

All emscripten targets that use Eigen failed at CMake configure time with:

```
CMake Error: Target "decision_tree_js" links to "Eigen3::Eigen" but the target was not found.
CMake Error: Target "distribution_js" links to "Eigen3::Eigen" but the target was not found.
... (12 targets total)
```

### Root cause

`Eigen3::Eigen` is an **imported CMake target** created by `find_package(Eigen3)`.
When cross-compiling with emscripten, the host's CMake package registry is not
searched, so `Eigen3::Eigen` is never defined and all `target_link_libraries` calls
that reference it fail.

---

## Fix

Since Eigen is a **header-only** library, no linking step is needed at all.
Replace every `target_link_libraries(X PRIVATE Eigen3::Eigen)` call with a direct
`target_include_directories` pointing at the bundled Eigen source tree.

### 1. Declare the include variable (after COOLBOX_ROOT)

```cmake
# Eigen is header-only; set include dir directly (find_package fails for cross-compilation)
set(EIGEN3_INCLUDE_DIR "${COOLBOX_ROOT}/external/eigen/eigen-3.4.0")
```

### 2. Bake it into the `add_emscripten_module` macro

```cmake
target_include_directories(${NAME} PRIVATE
    ${INCLUDE_DIRS} ${METADATA_HEADERS_DIR} ${MATRIX_HEADERS_DIR}
    ${PACKAGES_ROOT} ${COOLBOX_ROOT}/_deliverables/libraries
    ${EIGEN3_INCLUDE_DIR}   # <-- added
)
```

All targets built through the macro now automatically get Eigen headers.

### 3. Fix manually-declared targets (circuitry_js)

```cmake
# Before (broken):
target_link_libraries(circuitry_js PRIVATE Eigen3::Eigen)

# After:
target_include_directories(circuitry_js PRIVATE "${EIGEN3_INCLUDE_DIR}")
```

### 4. Remove all per-target `if(TARGET X) ... Eigen3::Eigen ... endif()` blocks

These 11 blocks are now redundant (Eigen is already included via the macro):

```cmake
# Removed for each of:
#   decision_tree_js, hidden_markov_model_js, distribution_js, glm_js,
#   deep_learning_js, svm_js, dimensionality_reduction_js,
#   latent_sentiment_analysis_js, marked_point_process_js,
#   tracker_js, time_series_js
if(TARGET <name>)
    target_link_libraries(<name> PRIVATE Eigen3::Eigen)
endif()
```

---

## Affected Targets

`circuitry_js`, `decision_tree_js`, `hidden_markov_model_js`, `distribution_js`,
`glm_js`, `deep_learning_js`, `svm_js`, `dimensionality_reduction_js`,
`latent_sentiment_analysis_js`, `marked_point_process_js`, `tracker_js`,
`time_series_js`
