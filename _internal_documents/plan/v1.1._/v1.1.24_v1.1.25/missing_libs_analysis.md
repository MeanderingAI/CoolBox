# Missing or Skipped Libraries Analysis

This document lists each library target that was skipped or missing in the build process, along with the likely reason for its absence.

---

## 1. password_hash_utils
- **Status:** Directory missing
- **Reason:** The folder `_libraries/packages/SECURITY/password_hash_utils` does not exist. It may have been deleted, renamed, or never implemented.

## 2. plpython_parser_lib, pljava_parser_lib, plrust_parser_lib
- **Status:** Directory missing
- **Reason:** No folders found for these parser libraries under `_libraries/packages/PARSER/`. They may have been renamed, removed, or not yet implemented.

## 3. plpython_lsp_lib, pljava_lsp_lib, plrust_lsp_lib, plvhdl_lsp_lib, plmatlab_lsp_lib
- **Status:** Some present, some missing/empty
- **Reason:** Some LSP libraries (e.g., plvhdl_lsp_lib) have CMakeLists.txt and sources, but others are missing their folders or CMake files. They may have been renamed, removed, or not yet implemented.

## 4. json
- **Status:** Directory missing
- **Reason:** The folder `_libraries/packages/IO/json` does not exist. It may have been renamed, deleted, or never implemented.

## 5. data_structures
- **Status:** Present
- **Reason:** The main library exists at `_libraries/packages/DATASTRUCTURE/` with a valid CMakeLists.txt and is correctly referenced in the build. A legacy stub existed at `_libraries/packages/IO/data_structures/` (single file, no CMakeLists.txt), which has now been removed to prevent confusion. Only the main DATASTRUCTURE library is used.

## 6. auth, network_scanner, malware_scanner, fuzzer
- **Status:** Present
- **Reason:** These folders and CMakeLists.txt exist. If skipped, check for conditional build logic or missing dependencies.

## 7. wave_generator_utils, metadata_management, thread_pool_utils, system_stats
- **Status:** Present
- **Reason:** These folders and CMakeLists.txt exist. If skipped, check for conditional build logic or missing dependencies.

## 8. Other ML/AI/Parser/Utils libraries (e.g., gabor_patches, deep_learning, decision_tree, etc.)
- **Status:** Present
- **Reason:** Folders and CMakeLists.txt exist. If skipped, check for conditional build logic or missing dependencies.

## 9. Libraries in ___DISABLED_binaries
- **Status:** Intentionally disabled
- **Reason:** These are in a folder prefixed with `___DISABLED`, so they are not included in the build by design.

---

**General Notes:**
- If a library is missing, check if it was renamed, moved, or deleted in recent commits.
- If a folder exists but is skipped, check for typos, CMake conditional logic, or missing dependencies.
- Disabled folders (like ___DISABLED_binaries) are not built by design.

---

This plan can be updated as libraries are restored, renamed, or intentionally removed.