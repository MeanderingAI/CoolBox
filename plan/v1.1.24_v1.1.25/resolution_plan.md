# Resolution Plan for Missing/Skipped Libraries

## 1. password_hash_utils
- **Previous Status:** Directory missing
- **Resolution:** The functionality for `password_hash_utils` appears to have been moved to `_libraries/backages/MISC/hash`.
- **Action:** Update CMake and dependent code to use `_libraries/backages/MISC/hash` instead of the old path. No further action needed unless specific features are still missing.

## 2. plpython_parser_lib, pljava_parser_lib, plrust_parser_lib, plpython_lsp_lib, pljava_lsp_lib, plrust_lsp_lib, plvhdl_lsp_lib, plmatlab_lsp_lib
- **Previous Status:** Some directories missing, some present but skipped
- **Resolution:** All these libraries exist, but their parent CMakeLists.txt entries were not clearly mapped to their actual library names. The parent _libraries/CMakeLists.txt has been updated to clarify and ensure correct add_subdirectory_if_exists calls for each renamed or present parser and LSP library, with comments indicating the actual library names as defined in each subdirectory's CMakeLists.txt.
- **Action:** No further action needed unless additional renames or moves occur. If a library is still skipped, check for typos or missing CMakeLists.txt in the relevant folder.

## 3. json
- **Previous Status:** Directory missing
- **Resolution:** The json library is now located at `_libraries/backages/IO/dataformats/json` instead of `_libraries/backages/IO/json`.
- **Action:** The parent CMakeLists.txt has been updated to use the correct path. No further action needed unless the library is moved again.


## 4. wave_generator_utils, metadata_management, thread_pool_utils, system_stats
- **Previous Status:** Present, but may be skipped
- **Resolution:**
	- All four libraries have valid folders and CMakeLists.txt under `_libraries/backages/MISC/`.
	- `wave_generator_utils` and `metadata_management` both use modern CMake patterns: C++17, public headers, and conditional GTest-based tests wrapped in `if(BUILD_TESTING)`.
	- `thread_pool_utils` is an INTERFACE library (header-only), included via `add_library(thread_pool_utils INTERFACE)` and exposes its headers.
	- `system_stats` is a standard shared library with public headers.
	- All are included in the parent `_libraries/CMakeLists.txt` via `add_subdirectory_if_exists` with no conditional logic or gating options.
- **Action:** No further action needed unless a folder or CMakeLists.txt is missing. If skipped, check for typos, missing dependencies, or build errors in the subdirectory.

Continue to update this file as more missing or renamed libraries are resolved.
