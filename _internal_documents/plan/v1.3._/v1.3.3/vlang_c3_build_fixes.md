# V and C3 Binding Build Fixes — v1.3.3

Fixes to `_scripts/build_scripts/build_extensions.py`, `vlang_bindings/coolbox.v`, and
`c3_bindings/src/coolbox.c3` to make V and C3 bindings compile correctly with their installed
compiler versions.

---

## 1. V compiler — wrong build command

### Problem
`v build .` requires the module to declare `module main`. The `vlang_bindings` module declares
`module coolbox` (a library), so V rejected it with:

```
project must include a `main` module or be a shared library (compile with `v -shared`)
```

### Fix — `build_extensions.py` — `_vlang_build()`
```python
# Before
return _run([v, "build", "."], binding_dir, binding_dir.name)

# After
return _run([v, "-shared", "."], binding_dir, binding_dir.name)
```

---

## 2. V compiler — linker can't find `coolbox_c_bindings`

### Problem
The V `#flag windows -L` directives only pointed to `c_bindings/build` (standalone build path).
The C bindings library is actually built by the main CMake project and lands in:
```
build/_deliverables/libraries/bindings/c_bindings/Debug/coolbox_c_bindings.dll
```

### Fix — `vlang_bindings/coolbox.v`
Added two additional `#flag windows -L` paths:
```v
#flag windows -L @VMODROOT/../../../../build/_deliverables/libraries/bindings/c_bindings/Debug
#flag windows -L @VMODROOT/../../../../build/_deliverables/libraries/bindings/c_bindings/Release
```

---

## 3. C3 compiler — `@cname` attribute removed in C3 0.6.5

### Problem
The installed compiler (C3 0.6.5) removed the `@cname` attribute. All five extern function
declarations in `coolbox.c3` used it:
```c3
extern fn ZString coolbox_c_version() @cname("coolbox_c_version");
```
Result: `Error: This is not a known valid attribute name`.

### Fix — `c3_bindings/src/coolbox.c3`
Removed all `@cname(...)` annotations. The extern names already matched the C function names, so
no alias was needed:
```c3
extern fn ZString coolbox_c_version();
```

---

## 4. C3 compiler — `tmem` allocator removed in C3 0.6.5

### Problem
`tmem` (temp-memory allocator) was removed in C3 0.6.5. Three string-return methods used it:
```c3
return value == null ? "" : value.copy(tmem);
```
Result: `Error: 'tmem' could not be found`.

### Fix — `c3_bindings/src/coolbox.c3`
Replaced `value.copy(tmem)` with `value.str_view()` (returns a `String` view of the C string
with no allocation needed; zero-copy and safe for returned literals):
```c3
return value == null ? "" : value.str_view();
```

---

## 5. C3 build command — `/noentry` + DLL linker error

### Problem
The build command `c3c compile --no-entry src/*.c3` triggered the LLVM linker in DLL mode but
without `/DLL`, causing:
```
lld-link: error: /noentry must be specified with /dll
```

### Fix — `build_extensions.py` — `_c3_build()`
Switched to `c3c static-lib` which produces a `.lib` without triggering the DLL linker step:
```python
return _run([c3c, "static-lib"] + [str(f) for f in c3_files], binding_dir, binding_dir.name)
```

---

## 6. C3 managed install not found by `_find_c3c()`

### Problem
`_find_c3c()` only checked PATH and hardcoded system paths. The GUI installs c3c to:
```
_local_build_pipeline/tmp/installers/c3c_install/c3-windows-Release/c3c.exe
```
This path was not checked, so builds always fell through to the "c3c not found" warning even
after a successful GUI install.

### Fix — `build_extensions.py` — `_find_c3c()`
Added `glob.glob` over the managed install dir (same pattern as `_find_v()`):
```python
managed_hits = glob.glob(str(managed / "**" / "c3c.exe"), recursive=True)
return _find_exec("c3c", [managed_hits[0] if managed_hits else "", ...])
```
