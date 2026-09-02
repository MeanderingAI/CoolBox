# GUI Package Builder — Extensions Tab

## Overview

Added a new **Extensions** top-level sub-tab to the Package Builder screen in `_interfaces/GUI/`. The tab lists all binding packages from `_deliverables/libraries/bindings/` and provides a per-binding **Build** button that streams live compiler output back to the browser.

---

## Files Changed

| File | Change |
|---|---|
| `_interfaces/GUI/static/screens/package_builder/package-builder.mjs` | Added `🧩 Extensions` tab; moved Middleware inside Client FE as inner sub-tab |
| `_interfaces/GUI/static/screens/package_builder/extension-builder.mjs` | New custom element `<extension-builder>` |
| `_interfaces/GUI/static/screens/package_builder/client-fe-viewer.mjs` | Added inner sub-tab bar: `🌐 Portals` / `🔧 Middleware` |
| `_interfaces/GUI/main.py` | New `GET /extensions` and `POST /extensions/build` endpoints |
| `_scripts/build_scripts/build_extensions.py` | New build script |

---

## Backend — `GET /extensions`

Scans `_deliverables/libraries/bindings/` and returns a flat list of binding folders with detected build type:

```json
{
  "bindings": [
    { "name": "c_bindings",      "lang": "C",      "build_type": "cmake" },
    { "name": "go_bindings",     "lang": "Go",     "build_type": "go"    },
    { "name": "scala_bindings",  "lang": "Scala",  "build_type": "maven" },
    { "name": "python_bindings", "lang": "Python", "build_type": "cmake" },
    { "name": "rust_bindings",   "lang": "Rust",   "build_type": "cargo" }
  ]
}
```

Detection priority: `CMakeLists.txt` → `Cargo.toml` → `go.mod` → `pom.xml` → `package.json` → `setup.py / pyproject.toml`.

---

## Backend — `POST /extensions/build`

Invokes `_scripts/build_scripts/build_extensions.py` for the requested binding and **streams** stdout+stderr back as `text/plain`. The last line is a sentinel:

```
__EXIT_CODE__:0
```

The frontend parses this to determine success/failure colouring after the stream closes.

### Windows asyncio workaround

`asyncio.create_subprocess_exec` raises `NotImplementedError` under uvicorn on Windows because uvicorn uses `SelectorEventLoop`, not `ProactorEventLoop`. The endpoint instead uses:

```python
loop.run_in_executor(None, _run_proc)  # blocking Popen in thread pool
loop.call_soon_threadsafe(queue.put_nowait, line)  # bridge to async queue
```

Each line produced by `Popen` is posted to an `asyncio.Queue` via `call_soon_threadsafe`, and the `StreamingResponse` generator awaits `queue.get()` until a `None` sentinel signals completion.

---

## Frontend — `<extension-builder>`

- Calls `GET /extensions` on load; renders one card per binding.
- Each card shows language icon, binding folder name, and detected build type.
- **Build** button streams the response body with `ReadableStream`:

```js
const reader = r.body.getReader();
while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    buffer += decoder.decode(value, { stream: true });
    out.textContent = buffer;         // live update
    out.scrollTop = out.scrollHeight; // auto-scroll
}
```

- `__EXIT_CODE__` sentinel stripped from visible output; used only to set `ok` / `err` CSS class on the `<pre>`.

---

## `_scripts/build_scripts/build_extensions.py`

New standalone script. Usage:

```
python build_extensions.py                     # build all bindings
python build_extensions.py python_bindings     # build one
python build_extensions.py --list              # list available
python build_extensions.py --target <target>   # override cmake target
```

Uses `Popen` with line-by-line stdout streaming and `flush=True` on all `print()` calls so output reaches the GUI immediately.
