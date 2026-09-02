# Binding Build Fixes

Fixes applied to the Extensions tab build pipeline to correctly handle all language binding types.

---

## Java (Maven) — `WinError 2` on Windows

**Problem:** `mvn.cmd package` raised `[WinError 2] The system cannot find the file specified` because
Maven was not on the system PATH.

**Fix:** Added `_find_maven()` helper in `_scripts/build_scripts/build_extensions.py`. It checks
`shutil.which("mvn")`, `shutil.which("mvn.cmd")`, and a set of common Windows install paths
(`C:\Program Files\Maven\bin\mvn.cmd`, `C:\tools\maven\bin\mvn.cmd`, etc.).
`_maven_build()` now emits a friendly error and install link rather than crashing:

```
Maven (mvn) not found. Download and install from https://maven.apache.org/download.cgi
Then add Maven's bin/ directory to your PATH and restart your terminal.
```

---

## Python — cmake target does not exist in main build

**Problem:** `python_bindings` contains both a `CMakeLists.txt` (using `pybind11_add_module`) and a
`setup.py` / `pyproject.toml`. The old detection logic chose cmake first, producing:

```
MSBUILD : error MSB1009: Project file does not exist.
Switch: python_bindings.vcxproj
```

The pybind11 cmake target is not registered in the main repo cmake build.

**Fix:** Reordered the build-system priority in `_build_binding()` so `has_py` (setup.py /
pyproject.toml) is checked **before** `has_cmake`. Python bindings now build via
`pip install -e .`. The same priority order is mirrored in the `GET /extensions` endpoint in
`main.py` so the UI shows `pip install` as the build type.

---

## Rust — workspace member path does not exist

**Problem:** Running `cargo build` from `_deliverables/libraries/bindings/rust_bindings/` caused
cargo to walk up to the repository root `Cargo.toml`, which declares:

```toml
[workspace]
members = ["_libraries/rust_bindings"]
```

That path (`_libraries/rust_bindings`) does not exist, so cargo failed:

```
error: failed to read `C:\KEYS\CoolBox\_libraries\rust_bindings\Cargo.toml`
Caused by: The system cannot find the path specified. (os error 3)
```

**Fix:** Added `[workspace]` to `_deliverables/libraries/bindings/rust_bindings/Cargo.toml`. This
makes the binding directory its own standalone workspace root, preventing cargo from ascending to
the repo root. `_cargo_build()` also now passes `--manifest-path` explicitly.

---

## C3 — no build system detected

**Problem:** `c3_bindings` contains only `.c3` source files and no `CMakeLists.txt`, `Cargo.toml`,
or other recognised build manifest. The detection logic fell through to "No recognised build system
found".

**Fix:**
- Added `_find_c3c()` helper checking common install paths (`C:\c3\c3c.exe`, etc.).
- Added `_c3_build()` which compiles `src/*.c3` with `c3c compile --no-entry`.
- Detection now checks `has_c3` via `glob("**/*.c3", recursive=True)` and sets `build_type="c3"`.
- `main.py` returns `has_c3c_exec` (bool) so the UI can show a warning badge if `c3c` is not
  installed.
- Install link shown when missing: `https://c3-lang.org`

---

## V (vlang) — no build system detected / `WinError 2`

**Problem:** `vlang_bindings` contains `*.v` source files and a `v.mod` manifest but no cmake or
cargo files. Detection matched `.v` files but then called `v build .` unconditionally, which raised
`[WinError 2]` when V was not on the PATH.

**Fix:**
- Added `_find_v()` helper checking `shutil.which("v")` and common Windows paths
  (`C:\V\v.exe`, `C:\tools\vlang\v.exe`, etc.).
- `_vlang_build()` now emits a friendly error when V is absent:
  ```
  V compiler not found. Download and install from https://vlang.io
  ```
- Detection also checks for `v.mod` (not just `*.v` in the root directory).
- `main.py` returns `has_v_exec` so the UI disables the Build button with a warning badge.

---

## R — no build system detected

**Problem:** `r_bindings` follows the standard R package layout (`DESCRIPTION`, `NAMESPACE`,
`src/`, `R/`) but none of those files were recognised by the build detector, so it fell through to
"No recognised build system found".

**Fix:**
- Added `_find_r()` helper checking `shutil.which("Rscript")`, `shutil.which("R")`, and versioned
  Windows install paths (`C:\Program Files\R\R-4.x.0\bin\Rscript.exe`).
- Added `_r_build()` which runs `R CMD build <dir>` from the parent directory.
- Detection now checks for the `DESCRIPTION` file as the canonical R package marker.
- `main.py` returns `has_r_exec` so the UI disables the Build button with a warning badge.
- Install link shown when missing: `https://cran.r-project.org`

---

## Frontend — toolchain warning badges

**Problem:** The Extensions tab showed a Build button for every binding regardless of whether the
required toolchain was installed.

**Fix (applied incrementally):** `extension-builder.mjs` now calls `_toolchainOk(binding)` and
`_toolchainHint(binding)` before rendering each card. When a required tool is absent the Build
button is **disabled** and a yellow warning badge is rendered below the binding metadata:

| Build type  | Badge text                                              |
|-------------|--------------------------------------------------------|
| emscripten  | ⚠️ emcc not found — install emsdk and activate it      |
| go          | ⚠️ go not found — install from https://go.dev/dl/      |
| maven       | ⚠️ Maven (mvn) not found — install from maven.apache.org |
| vlang       | ⚠️ V compiler not found — install from vlang.io        |
| c3          | ⚠️ c3c not found — install from c3-lang.org            |
| r           | ⚠️ R not found — install from cran.r-project.org       |

`BUILD_TYPE_LABEL` was also extended with `vlang`, `c3`, and `r` entries so the meta line displays
correctly (e.g. `v build`, `c3c compile`, `R CMD build`).
