# v1.4.8 README

## CI Fix: build_cpp_docs script path

### Issue

GitHub Actions `build_cpp_docs` failed with:

- `bash: ./_scripts/build_cpp_docs.sh: No such file or directory`

The build helper scripts were moved under `_scripts/build_scripts/`, but the workflow still referenced the old path.

### Fixes applied

1. Added/standardized C++ docs build script at:

- `_scripts/build_scripts/build_cpp_docs.sh`

2. Updated workflow path in:

- `.github/workflows/build-cpp-docs.yaml`

from:

- `bash ./_scripts/build_cpp_docs.sh "$GITHUB_WORKSPACE/cpp-docs-site"`

to:

- `bash ./_scripts/build_scripts/build_cpp_docs.sh "$GITHUB_WORKSPACE/cpp-docs-site"`

3. Updated local build pipeline callers for consistency:

- `_local_build_pipeline/scripts/jobs/build-cpp-docs.sh`
- `_local_build_pipeline/scripts/jobs/docs-publish-dry-run.sh`

### Validation

Local run:

- `bash ./_scripts/build_scripts/build_cpp_docs.sh "$(pwd)/cpp-docs-site-test"`

Result: script completed successfully and generated Doxygen HTML output.

## Related v1.4.8 notes

- This update aligns `build_cpp_docs` with the prior `build_scripts` path migration used by tutorial/docs scripts.
- Linux DataStructures `std::out_of_range` include fix is documented in `linux_data_structures_maple_tree_stdexcept_fix.md`.

## Windows Fix: os_dialog std::copy_n compile error

### Issue

Windows CI failed in `_deliverables/libraries/groups/app_builder/OS_GENERICS/os_dialog/source/os_dialog.cpp` with:

- `error: 'copy_n' is not a member of 'std'; did you mean 'copy'?`

### Root cause

`std::copy_n` was used in `show_native_win32(...)` without including the required standard header.

### Fix

Added:

- `#include <algorithm>`

to `os_dialog.cpp`, which provides `std::copy_n`.

### Validation

Local target build completed successfully:

- `cmake --build build --target os_dialog -j4`
- `BUILD_EXIT_CODE:0`

## macOS Fix: uuid_generation OpenSSL MD5 deprecation warning

### Issue

macOS build emitted deprecation warnings in `uuid_generation.cpp` with OpenSSL 3:

- `warning: 'MD5' is deprecated [-Wdeprecated-declarations]`

### Root cause

UUID hashing used deprecated low-level OpenSSL hash APIs (`MD5(...)`, and similarly `SHA1(...)` style usage).

### Fix

Migrated hashing in:

- `_deliverables/libraries/groups/trekker/MISC/uuid_generation/source/uuid_generation.cpp`

from low-level digest calls to EVP digest context flow:

- `EVP_MD_CTX_new`
- `EVP_DigestInit_ex`
- `EVP_DigestUpdate`
- `EVP_DigestFinal_ex`
- `EVP_MD_CTX_free`

MD5 and SHA-1 based UUID helper paths now use EVP-based implementations with existing deterministic fallback retained if digest operations fail.

### Validation

Local target build completed successfully:

- `cmake --build build --target uuid_generation -j4`
- `BUILD_EXIT_CODE:0`
