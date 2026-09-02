# Test Build Target Fix — v1.3.3

Fixes to `_interfaces/GUI/library/makefile_manager.py` and
`_interfaces/GUI/static/screens/test-runner.mjs` so the Build button in the test runner
always triggers the correct cmake target and users can see which target will be built.

---

## Problem

### 1. `_find_build_target()` returned library targets instead of test targets

`scan_tests()` called `_find_build_target(working_dir)` to decide which cmake target the
Build button should trigger.  The function listed `.vcxproj` files in the working directory
and returned the first alphabetically.

For tests whose working directory is the test subdirectory itself (rather than an isolated
`X_Y_build` directory), the directory contains both a library target and a test target, e.g.:

```
build/_deliverables/.../file_browser/
    file_browser_lib.vcxproj   ← alphabetically first → returned as build target (WRONG)
    file_browser_tests.vcxproj
```

Clicking Build on those rows triggered `cmake --build --target file_browser_lib`, which CMake
could not resolve, producing:

```
Error: No cmake target or package directory named 'file_browser_lib' found
```

**Affected tests (14+):** TystFrameworkTests, FileBrowserTests, BowerShellTests,
AudioMixerTests, VideoDisplayTests, GraphicsComponentsTests, ChartsTests, GraphicsFontsTests,
WindowsSimulationTests, FullApplicationWindowTests, DataStructuresTests,
MetadataManagementTests, DistributedTests, DistributedStorageTests, MusicSequencerTests,
MusicTheoryTests, NoteSynthesisTests, test_coolbox_c_bindings.

### 2. ctest output format caused working-dir / test-name misassignment

The `ctest -N --verbose` output prints the **Working Directory** line *before* the `Test #N:`
line:

```
N: Working Directory: /path/to/dir
  Test  #N: TestName
```

The original parser reset `current_workdir = None` when it saw `Test #N:`, then read the
next Working Directory line into `current_workdir`, and flushed the *previous* test using
that wrong path.  Result: every test's build target was looked up in the *next* test's
working directory.

### 3. No visual indicator of resolved cmake target in the UI

Users had no way to see which cmake target would be invoked, making mismatches hard to
diagnose.

---

## Fix

### `_interfaces/GUI/library/makefile_manager.py`

#### `_find_build_target(working_dir, test_name)`

- Added `test_name` parameter so the function can prefer the matching target.
- Uses `os.path.realpath()` to resolve `..` segments before listing vcxprojs.
- Collects all candidate stems (excluding `ALL_BUILD`, `ZERO_CHECK`, `INSTALL`, `RUN_TESTS`).
- If multiple candidates exist:
  1. Tries exact snake_case conversion of `test_name` (e.g. `FileBrowserTests` → `file_browser_tests`).
  2. Falls back to the single target whose stem contains `test`.
  3. Last resort: prefix match against snake_case name.
- Helper `_to_snake(s)` converts PascalCase → snake_case using two regex substitutions.

#### `scan_tests()` — working-dir assignment fix

- Introduced `prev_workdir` to track the working dir that belongs to the *previous* test.
- When `Test #N:` is seen, `current_workdir` holds test N's dir (just read); flush the
  *previous* test using `prev_workdir`, then promote `current_workdir → prev_workdir`.
- This correctly pairs each test name with its own working directory.

#### `build_target()` — snake_case fallback

- If the cmake target is not found directly in `cmake_deps`, converts it from PascalCase to
  snake_case and retries before returning "not found".

### `_interfaces/GUI/static/screens/test-runner.mjs`

- Each test row now shows the resolved cmake target as a small grey monospace hint beneath
  the test name:

```
FileBrowserTests
cmake: file_browser_tests
```

Implementation: replaced the single `nameCell.textContent = test.name` with a `<span>` for
the name and a `<div>` (font-size 0.75em, color #aaa) for the cmake target if present.

---

## Verification

After the fix, `scan_tests()` returns correct `build_target` for all 64 registered CTest tests.
Previously problematic examples:

| Test Name | Before | After |
|---|---|---|
| TystFrameworkTests | file_browser_tests (wrong dir!) | tyst_framework_tests |
| FileBrowserTests | file_browser_lib | file_browser_tests |
| BowerShellTests | bower_shell_lib | bower_shell_tests |
| GraphicsComponentsTests | components_lib | components_tests |
| DataStructuresTests | data_structures_lib | data_structures_tests |
