# Toolchain Binding Detection Fix (C3 / V) — v1.3.3

## Problem

After installing the C3 or V compiler via the GUI Install Tools panel, the Extensions tab would
show a **green checkmark** for those tools in the Install Tools list (correct), but the
corresponding binding card still displayed the warning:

```
⚠️ c3c not found — use Install Tools or https://c3-lang.org
⚠️ V compiler not found — use Install Tools or https://vlang.io
```

## Root Cause

Two code paths check for the same tools but used different detection logic:

| Endpoint | Detection used |
|---|---|
| `GET /extensions/tools` (`list_tools`) | `shutil.which` + hardcoded system paths + **managed install glob** |
| `GET /extensions` (per-binding scan) | `shutil.which` + hardcoded system paths only |

`master_installer.py` installs c3c and vlang into the **managed installer directory**:
```
_local_build_pipeline/tmp/installers/c3c_install/**/c3c.exe
_local_build_pipeline/tmp/installers/vlang_install/**/v.exe
```
`list_tools()` already used `glob.glob(..., recursive=True)` to find these. The per-binding scan
in the `/extensions` endpoint did not, so it always returned `has_c3c_exec: false` /
`has_v_exec: false` for managed installs.

## Fix — `_interfaces/GUI/main.py` — `GET /extensions`

Extended both per-binding checks to also scan the managed installer directories:

**V compiler (`has_v_exec`):**
```python
import glob as _glob
_repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_v_exe = "v.exe" if os.name == "nt" else "v"
_v_managed = _glob.glob(
    os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                 "vlang_install", "**", _v_exe), recursive=True)
has_v_exec = bool(
    _shutil.which("v") or
    any(os.path.isfile(p) for p in [r"C:\V\v.exe", r"C:\tools\vlang\v.exe"]) or
    _v_managed
)
```

**C3 compiler (`has_c3c_exec`):**
```python
import glob as _glob
_repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_c3c_exe = "c3c.exe" if os.name == "nt" else "c3c"
_c3c_managed = _glob.glob(
    os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                 "c3c_install", "**", _c3c_exe), recursive=True)
has_c3c_exec = bool(
    _shutil.which("c3c") or
    any(os.path.isfile(p) for p in [r"C:\c3\c3c.exe", r"C:\tools\c3\c3c.exe"]) or
    _c3c_managed
)
```

## Detection Priority (after fix)

1. `shutil.which(name)` — executable on system PATH
2. `os.path.isfile(p)` for known system install locations
3. `glob.glob(...)` in `_local_build_pipeline/tmp/installers/` — managed GUI installs

This matches the detection order already in `list_tools()`, eliminating the inconsistency.

## Related

- [toolchain_install_status_fix.md](toolchain_install_status_fix.md) — earlier fix that added
  glob detection to `list_tools()`; this fix brings the per-binding scan to parity.
