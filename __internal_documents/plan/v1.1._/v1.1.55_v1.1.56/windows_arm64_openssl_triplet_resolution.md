# Windows ARM64 OpenSSL Triplet Resolution

## Related Notes
- Follow-on change to `plan/v1.1.52_v1.1.53/windows_arm64_vcpkg_bootstrap.md` for ARM64 hosted-runner dependency bootstrap.
- Follow-on change to `plan/v1.1.53_v1.1.54/windows_arm64_cmake_toolchain_argument_passing.md` for the ARM64 PowerShell CMake configure path.
- Follow-on change to `plan/v1.1.54_v1.1.55/windows_arm64_bison_path_and_detection.md` for host-tool dependency resolution during ARM64 configure.

## Summary
- Fixed the Windows ARM64 workflow so OpenSSL is installed for the `arm64-windows` triplet instead of falling through to a host x64 installation.
- Updated Windows dependency discovery so CMake prefers the triplet-matched `vcpkg` OpenSSL root when OpenSSL is available there.
- Removed the architecture mismatch that linked ARM64 targets against `C:\Program Files\OpenSSL\lib\VC\x64` libraries.

## Problem
- Several Windows libraries, including `cryptocurrency_utils`, rely on `find_package(OpenSSL)` and `OpenSSL::Crypto`.
- The ARM64 workflow installed GSL, SQLite, and Eigen through `vcpkg`, but did not install `openssl:arm64-windows`.
- Without an ARM64 OpenSSL package in the active triplet, CMake could fall back to a machine-wide x64 OpenSSL installation under `C:\Program Files\OpenSSL`.
- That produced linker failures such as `LNK4272` machine-type conflicts and unresolved OpenSSL symbols like `PEM_read_bio_PUBKEY` and `PEM_write_bio_PUBKEY` while building ARM64 targets.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `cmake/FindAllDependencies.cmake`

## Change
- Updated the Windows ARM64 `vcpkg` install step in both workflows to include `openssl:arm64-windows` alongside the existing ARM64 dependency set.
- Extended Windows dependency discovery in `FindAllDependencies.cmake` so when the active `vcpkg` triplet contains OpenSSL headers, `OPENSSL_ROOT_DIR` is set to that triplet root.
- Kept the existing triplet-aware `CMAKE_PREFIX_PATH` setup so `find_package(OpenSSL)` resolves within the same ARM64 dependency tree as the rest of the cross-compile dependencies.

## Result
- Windows ARM64 hosted builds can now link OpenSSL-dependent targets against ARM64 OpenSSL artifacts from `vcpkg`.
- The workflow no longer needs to fall back to an incompatible host x64 OpenSSL installation.
- ARM64 targets that depend on `OpenSSL::Crypto` should no longer fail with x64-vs-ARM64 linker conflicts caused by mismatched library architecture.