# Emscripten Bindings — Source Path Fixes

## Overview

`emscripten_bindings/CMakeLists.txt` contained incorrect paths for all source files
and include directories, caused by two compounding bugs.

---

## Fix 1: COOLBOX_ROOT resolved to the wrong directory

**File:** `_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt`

### Problem

The original line:
```cmake
get_filename_component(COOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
```

`CMAKE_CURRENT_SOURCE_DIR` is `_deliverables/libraries/bindings/emscripten_bindings/`.
Going `../..` reaches `_deliverables/libraries/` — **not** the repo root.

This caused all `${COOLBOX_ROOT}/_libraries/...` paths to resolve to:
```
_deliverables/libraries/_libraries/...   ← does not exist
```

### Fix
Use `../../../../` (4 levels up) to reach the repo root:
```cmake
get_filename_component(COOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
```

---

## Fix 2: Package paths used a legacy `_libraries/packages/` prefix

Source groups were moved to `_deliverables/libraries/groups/` but CMakeLists still
referenced the old layout. Correct path variable mappings:

| Variable | Old (broken) | New (correct) |
|---|---|---|
| `ML_ROOT` | `_libraries/packages/ML` | `_deliverables/libraries/groups/cool_car/ML` |
| `DL_ROOT` | `_libraries/packages/DL` | `_deliverables/libraries/groups/cool_car/DL` |
| `DS_SRC_DIR` | `_libraries/packages/DATASTRUCTURE` | `_deliverables/libraries/groups/trekker/DATASTRUCTURE` |
| `GP_SRC_DIR` | `_libraries/packages/ML/gabor_patches` | `_deliverables/libraries/groups/cool_car/ML/gabor_patches` |
| `BATTERY_SRC_DIR` | `_libraries/packages/ELECTRONICS/battery` | `_deliverables/libraries/groups/sim_group/ELECTRONICS/battery` |
| `PACKAGES_ROOT` | `_libraries/packages` | `_deliverables/libraries/groups` |
| `METADATA_HEADERS_DIR` | `_libraries/packages/MISC/metadata_management/headers` | `_deliverables/libraries/groups/trekker/MISC/metadata_management/headers` |
| `MATRIX_HEADERS_DIR` | `_libraries/packages/DATASTRUCTURE/matrix/headers` | `_deliverables/libraries/groups/trekker/DATASTRUCTURE/matrix/headers` |
| `ADV_LOG_SRC_DIR` | *(undefined)* | `_deliverables/libraries/groups/trekker/IO/advanced_logging` |
| Inline `_libraries/include` | `_libraries/include` | `_deliverables/libraries` |
| Inline chemistry include | `_libraries/packages/CHEMISTRY/include` | `_deliverables/libraries/groups/sim_group/CHEMISTRY/include` |

---

## Fix 3: Deep Learning subdirectory layout mismatch

The Deep Learning group does not have a monolithic `deep_learning/source/` directory.
Sources are split across four subdirectories under `DL/`:

| Source file | Old path (broken) | Actual location |
|---|---|---|
| `tensor.cpp`, `layer.cpp` | `DL/deep_learning/source/` | `DL/layers/src/` |
| `loss.cpp` | `DL/deep_learning/source/` | `DL/loss/src/` |
| `optimizer.cpp` | `DL/deep_learning/source/` | `DL/optimizer/src/` |
| `neural_network.cpp`, `templates.cpp` | `DL/deep_learning/source/` | `DL/wrapper/src/` |

Headers also changed: each subdirectory uses `include/` not `headers/`.
The corrected include dirs are:
```cmake
"${DL_ROOT}/layers/include;${DL_ROOT}/loss/include;${DL_ROOT}/optimizer/include;${DL_ROOT}/wrapper/include"
```
