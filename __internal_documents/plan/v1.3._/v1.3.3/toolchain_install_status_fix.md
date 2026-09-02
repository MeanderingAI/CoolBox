# Toolchain Install Status Fix — v1.3.3

## Problem
After successfully installing a tool (e.g. Go) via the Install Tools panel, clicking **↻ Re-check** or waiting for the automatic post-install refresh did not un-grey the tool row. The tool remained shown as **✗ missing** even though it was installed correctly.

## Root Cause
`master_installer.py` installs tools into a managed directory:
```
_local_build_pipeline/tmp/installers/<tool>_install/...
```
For example, Go lands at:
```
_local_build_pipeline/tmp/installers/go_install/go/bin/go.exe
```
The `GET /extensions/tools` endpoint in `main.py` checked:
1. `shutil.which(name)` — only finds executables on the system `PATH`
2. A hardcoded `extra` list of typical system-wide install paths (`C:\Program Files\Go\...`, etc.)

Neither of these includes the managed installer paths, so the check always returned `installed: false` for locally managed installs.

## Fix — `_interfaces/GUI/main.py` — `list_tools()`
Rewrote the detection logic to also scan the managed install directories:

| Tool | Detection method |
|------|-----------------|
| **Go** | Fixed path: `go_install/go/bin/go.exe` — always deterministic |
| **Maven** | `glob("maven_install/apache-maven-*/bin/mvn.cmd")` — version varies |
| **c3c** | `glob("c3c_install/**/c3c.exe", recursive=True)` |
| **vlang** | `glob("vlang_install/**/v.exe", recursive=True)` |
| **R** | System installer only; existing `C:\Program Files\R\R-*` extras unchanged |
| **emsdk** | Fixed path: `emsdk/upstream/emscripten/emcc.bat` — already correct |

Added a `"glob"` key to affected tool entries; the check function `_glob_managed(*pattern)` returns `True` if any file matching the glob exists under `_local_build_pipeline/tmp/installers/`.

## Detection priority (for each tool)
1. `shutil.which(name)` — system PATH
2. `os.path.isfile(p)` for each path in `extra` — system-wide known locations
3. `glob.glob(...)` inside `_local_build_pipeline/tmp/installers/` — managed installs

## Frontend behaviour (unchanged)
`_installTool()` in `extension-builder.mjs` already called `await this._loadTools()` and `await this._load()` after a successful install (exit code 0). With the backend fix the re-check now correctly returns `installed: true` and the row turns green immediately.
