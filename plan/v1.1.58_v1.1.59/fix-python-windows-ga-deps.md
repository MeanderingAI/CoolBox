# Python Windows Build: GA Pipeline Dependency Fixes

- Problem: The GitHub Actions pipeline for Python/Windows fails due to missing dependencies: GSL (GNU Scientific Library) and Bison (parser generator).
- Solution:
  - For GSL: Install with vcpkg (`vcpkg install gsl`) and ensure the vcpkg toolchain is used or add the GSL CMake path to `CMAKE_PREFIX_PATH`.
  - For Bison: Run the provided script (`powershell -ExecutionPolicy Bypass -File _scripts/install_bison.ps1`) or manually install winflexbison and add to PATH.
- Impact: Ensures all required dependencies are present for CMake configuration and build, allowing the pipeline to proceed.
- Location: See CMake error output and `_scripts/install_bison.ps1` for details.
- Example commands:
  ```sh
  vcpkg install gsl
  powershell -ExecutionPolicy Bypass -File _scripts/install_bison.ps1
  ```
- This step is required for all Windows builds in CI and local development.
