# Fix Rust Windows DLL Not Found in Tests

- Problem: Rust tests on Windows fail with STATUS_DLL_NOT_FOUND because the required C bindings DLLs are not found at runtime.
- Solution: The workflow now copies all C bindings DLLs into the Rust test binary directory (`target/release/deps/`) before running tests on Windows.
- Impact: Ensures Rust tests can find and load the required DLLs, resolving the test failure and enabling CI to pass on Windows.
- Location: See `.github/workflows/generate-purchase-rust.yaml` for the new step.
- Example step added:
  ```sh
  cp _libraries/c_bindings/build/*.dll target/release/deps/ || true
  cp _libraries/c_bindings/build/Debug/*.dll target/release/deps/ || true
  cp _libraries/c_bindings/build/Release/*.dll target/release/deps/ || true
  ```
- This step is only run on Windows runners.
