## 2026-04-19: ARM64 build fixes for CMake and MSYS2

- Disabled MSYS2 install for ARM64 (assume preinstalled or handled outside workflow)
- Updated CMake configure step to:
  - Use PowerShell for correct variable expansion
  - Clean build dir before switching generators
  - Fail early if vcpkg toolchain file is missing
  - Try MinGW Makefiles first, fallback to MSVC if needed
  - This should resolve toolchain and generator mismatch errors
