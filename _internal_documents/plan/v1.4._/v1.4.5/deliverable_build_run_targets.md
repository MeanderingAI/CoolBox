# Deliverable build / run / package Makefile targets

**Version:** v1.4.5  
**Date:** June 27 2026  
**Status:** complete

## Goal

Provide a consistent Makefile syntax for building, running, and packaging CoolBox deliverables from `_deliverables/apps` and `_deliverables/libraries`, without needing to remember raw CMake target names.

## Makefile syntax

| Command | Description |
|---------|-------------|
| `make build app NAME` | Build one app and its CMake dependencies |
| `make build library NAME [NAME...]` | Build a library group or individual library targets |
| `make package library NAME [NAME...]` | Build libraries and copy artifacts into `lib/packages/` |
| `make run app NAME [TARGET]` | Run a built app (auto-builds if missing) |
| `make list_apps` | List apps under `_deliverables/apps` |
| `make list_libraries` | List libraries under `_deliverables/libraries` |

Examples:

```bash
make build app battery_simulator
make build library audio_visual_group
make build library audio_processing video_codec
make package library audio_processing video_codec
make run app battery_simulator -- -rt
make run app battery_simulator -- --random-test
make run app battery_simulator -- -s examples/basic_pack.bsim
```

Works on macOS (`Makefile.osx`), Linux (`Makefile.lin`), and Windows (`Makefile.win`).

## Where built artifacts go

### Apps (CMake executables)

Apps land under the CMake build tree, mirroring their source path:

| Platform | Path |
|----------|------|
| macOS / Linux | `build/_deliverables/apps/<app_folder>/<executable_name>` |
| Windows | `build/_deliverables/apps/<app_folder>/Release/<executable_name>.exe` (or `Debug/`) |

Example — `battery_simulator`:

```
build/_deliverables/apps/battery_simulator/battery_simulator
```

Multi-target apps (e.g. `lsp`, `database_driver`) produce one binary per `add_executable` target in the same app folder.

### Libraries

Shared/static libraries are written under `build/_deliverables/libraries/…`, following the same directory layout as source. Exact suffix depends on platform (`.dylib`, `.so`, `.dll`, `.a`, `.lib`).

### Runtime library path

- `make run app …` prepends `./lib` to `DYLD_LIBRARY_PATH` (macOS), `LD_LIBRARY_PATH` (Linux), or `PATH` (Windows) when that folder exists.
- `make install` (Windows Makefile) copies built DLLs into `./lib`.
- `make package library …` collects selected library artifacts into `lib/packages/<name>/`.

## Scripts (`_scripts/`)

| Script | Role |
|--------|------|
| `deliverable_utils.py` | Shared discovery: map app/library names → CMake targets |
| `list_deliverables.py` | `make list_apps` / `make list_libraries` backend |
| `build_deliverable.py` | `make build app/library …` backend |
| `run_deliverable.py` | `make run app …` backend |
| `package_libraries.py` | `make package library …` backend |

## Name resolution

- **App folder name** — e.g. `battery_simulator` → executable target `battery_simulator`
- **Library group directory** — e.g. `audio_visual_group` → all `add_library` targets under that tree
- **Individual CMake target** — e.g. `audio_processing`, `video_codec`
- **Multi-exec apps** — `make run app lsp plang_lsp` selects a specific executable; omitting `TARGET` uses the target whose name matches the app folder, or the sole target if there is only one

Python-only apps (no `CMakeLists.txt`) are run via the repo Python interpreter from their folder in `_deliverables/apps/<name>/`.

## Implementation notes

- Make treats `app`, `library`, and app/library names as dummy goals so `make build app battery_simulator` parses correctly.
- `run_deliverable.py` builds automatically when the binary is missing unless `--no-build` is passed.
- Windows targeted builds still run `sign_windows_artifacts.ps1` after `build` and `package`.
- The GUI app launcher (`_interfaces/GUI/routes/apps.py`) uses the same `build/` search logic via `makefile_manager.find_artifacts`.

## Follow-ups (optional)

- Wire GUI Build/Launch buttons to the new `make build app` / `make run app` targets.
- Add `make run app NAME -- ARGS…` passthrough for CLI arguments.
- Add tab-completion for app and library names.
