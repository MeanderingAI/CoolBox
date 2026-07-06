# R Bindings Build Fix — v1.3.3

Fixes to `_scripts/build_scripts/build_extensions.py` to make the R binding (`coolboxr`) build
successfully on Windows with Rtools45 installed in a non-system location.

---

## 1. `\K` escape error in R `-e` inline expression

### Problem
The original `_r_build()` passed the package path as an R `-e` inline expression:

```python
[r_exe, "-e", f'devtools::build("{pkg_dir}")']
```

On Windows `pkg_dir` is `C:\KEYS\CoolBox\...`. Backslashes inside an R string literal are escape
sequences (`\K` → unknown escape, error).

### Fix
Switched from the `-e` inline approach to `R CMD build`, which accepts the path as a plain
command-line argument:

```python
[r_exe, "CMD", "build", str(pkg_dir)]
```

- `r_exe` is derived from Rscript's sibling file (`Path(rscript).with_name("R.exe")`), which
  works for any installed R version without hardcoding a version string.
- The command is passed as an `argv` list, so no shell interpolation of backslashes occurs.

---

## 2. `R CMD build` only produces a source tarball

### Problem
`R CMD build` creates `coolboxgui_0.1.0.tar.gz` (source package). A source package cannot be
`library()`-loaded on Windows; a binary `.zip` (or installed package) is required.

### Fix — two-step build
Added a second step after `R CMD build`:

```python
# Step 2: install locally + produce binary .zip
[r_exe, "CMD", "INSTALL", "--build", str(pkg_dir)]
```

`R CMD INSTALL --build` compiles the package and emits `coolboxgui_0.1.0.zip` (Windows binary)
in addition to installing it into the user library. Both artifacts are kept.

---

## 3. Rtools not found — R's internal PATH resolves to system path

### Problem
R internally prepends `c:/rtools45/usr/bin` to PATH when it starts. On this machine Rtools45 is
installed to `~\rtools45\` (user home), not `C:\rtools45\`. R's internal PATH therefore points
nowhere, and `R CMD INSTALL --build` fails with "make not found" / "gcc not found".

The `RTOOLS45_HOME` / `RTOOLS_HOME` environment variables would normally tell R where Rtools is,
but they were not set.

### Fix — Rtools environment injection
Detect the Rtools root using `_find_rtools_root()` (checks `RTOOLS45_HOME`, `RTOOLS_HOME`, and
common locations including `~\rtools45\`), then inject into the env passed to `_run()`:

```python
env["RTOOLS45_HOME"] = str(rtools_root)
env["RTOOLS_HOME"]   = str(rtools_root)
env["PATH"]          = (
    str(rtools_root / "x86_64-w64-mingw32.static.posix" / "bin") + os.pathsep +
    str(rtools_root / "usr" / "bin") + os.pathsep +
    env.get("PATH", os.environ.get("PATH", ""))
)
env.setdefault("R_LIBS_USER", str(Path.home() / "R" / "win-library"))
```

The `R_LIBS_USER` default avoids needing admin rights to install to the system R library.

---

## 4. `_run()` does not support environment injection

### Problem
`_run()` called `subprocess.Popen` without an `env` argument, so env vars could not be injected
per-build without modifying the global environment.

### Fix — `extra_env` parameter
Added `extra_env: dict | None = None` to `_run()`. When provided, a copy of `os.environ` is made,
updated with `extra_env`, and passed as the `env` argument to `subprocess.Popen`.

---

## Outcome

After all fixes, `_r_build()` produces:
- `coolboxgui_0.1.0.tar.gz` — source package (from `R CMD build`)
- `coolboxgui_0.1.0.zip` — Windows binary package (from `R CMD INSTALL --build`)

Both are emitted to the directory containing `coolboxr/` (i.e. the `bindings/` folder).
