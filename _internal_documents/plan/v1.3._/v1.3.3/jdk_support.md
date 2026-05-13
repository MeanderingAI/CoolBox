# JDK Support — v1.3.3

Add JDK 21 LTS (Eclipse Temurin) as a recognised, installable, and warnable tool — required by
Maven to build the Java bindings.

---

## Problem

`java_bindings` uses Maven (`pom.xml`). Maven requires a JDK. When Java was not installed, Maven
exited immediately with:

```
The JAVA_HOME environment variable is not defined correctly,
this environment variable is needed to run this program.
```

Neither the build script nor the GUI had any awareness of JDK; the user had no path to resolution.

---

## Files Changed

### 1. `_scripts/build_scripts/build_extensions.py`

Added two helpers:

- **`_find_java_home()`** — returns the JDK root directory (for `JAVA_HOME`):
  1. Honours existing `JAVA_HOME` env var if it points to a real `javac`
  2. Globs managed install (`jdk_install/jdk-*/bin/javac.exe`)
  3. Scans common Windows system paths (`C:\Program Files\Java\jdk-*`, Eclipse Adoptium,
     Microsoft OpenJDK)

- **`_find_java()`** — returns the `java.exe` path using `_find_java_home()` or `shutil.which`.

Updated **`_maven_build()`**:
- Checks `_find_java_home()` before running mvn; emits `[WARN] JDK not found` and skips if absent.
- Injects `JAVA_HOME` and prepends `<jdk>/bin` to `PATH` in the env passed to `_run()`.

### 2. `_scripts/install_scripts/master_installer.py`

Added `# ── JDK (Eclipse Temurin 21 LTS) ──` section:

- **`_jdk_install_dir()`** — `_tmp_dir() / "jdk_install"`
- **`_jdk_java_exe()`** — globs `jdk_install/jdk-*/bin/java.exe`
- **`_jdk_installed()`** — checks PATH, managed install, and system `C:\Program Files\Java\...`
- **`_jdk_url()`** — Eclipse Temurin API URL for JDK 21 GA (Windows/Linux/macOS)
- **`install_jdk()`** — downloads `.zip` (Windows) or `.tar.gz` (Linux/macOS), extracts to
  `jdk_install/`, sets `JAVA_HOME` and updates `PATH` for the current process

Added `"jdk"` to the `TOOLS` registry:
```python
"jdk": {"label": "JDK 21 (Temurin)", "fn": install_jdk, "check": ("java",)}
```

Added `if tool_key == "jdk": return _jdk_installed()` branch to `_is_installed()`.

### 3. `_interfaces/GUI/main.py` — `list_tools()` (`GET /extensions/tools`)

Added `"jdk"` to the `TOOLS` dict with:
- `check`: `["java"]`
- `extra`: system-wide `java.exe` paths
- `glob`: `("jdk_install", "jdk-*", "bin", "java.exe")` — managed install

### 4. `_interfaces/GUI/main.py` — `GET /extensions` (per-binding response)

Added `has_java_exec` detection alongside `has_mvn_exec` (both computed when `has_pom` is true):

```python
_jdk_managed = glob.glob(os.path.join(..., "jdk_install", "jdk-*", "bin", "java.exe"))
has_java_exec = bool(
    shutil.which("java") or _jdk_managed or _jdk_system or os.environ.get("JAVA_HOME")
)
```

`has_java_exec` included in the per-binding response dict.

### 5. `_interfaces/GUI/main.py` — `VALID_TOOLS`

Added `"jdk"` to the set so `POST /extensions/install` accepts `{ "tool": "jdk" }`.

### 6. `_interfaces/GUI/static/screens/package_builder/extension-builder.mjs`

Added to `_toolchainHint()`:
```javascript
if (binding.build_type === 'maven' && binding.has_java_exec === false)
    return '⚠️ JDK not found — Maven requires Java; use Install Tools to install JDK 21';
```

Shown only when Maven is present but JDK is absent (separate from the Maven-not-found warning).

---

## Detection Priority (JDK)

1. `shutil.which("java")` — system PATH
2. `os.environ["JAVA_HOME"]` — environment variable (any source)
3. `jdk_install/jdk-*/bin/java.exe` — managed GUI install
4. `C:\Program Files\Java\jdk-*`, Eclipse Adoptium, Microsoft OpenJDK — system installs

---

## Notes

- Eclipse Temurin is chosen because it has a stable REST API for fetching the latest GA release
  without scraping.
- The JDK is installed to the managed dir (no admin required), and `JAVA_HOME` + `PATH` are
  injected into the subprocess environment at build time — no system-level PATH change needed.
