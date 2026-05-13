# GUI Path & Layout Fixes

Fixes applied to the `_interfaces/GUI/` dashboard after the GUI was moved from the repo root into `_interfaces/GUI/`.

---

## 1. Wrong `repo_root` depth in `main.py`

**Problem:** All `repo_root` calculations in `main.py` used two `os.path.dirname` calls:

```python
# resolves to _interfaces/ — wrong
repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
```

`main.py` lives at `_interfaces/GUI/main.py`, so two levels up is `_interfaces/`, not the repo root `CoolBox/`.

**Fix:** All 32+ occurrences updated to three `dirname` calls:

```python
# resolves to CoolBox/ — correct
repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
```

---

## 2. Wrong `_repo_root()` depth in `makefile_manager.py`

**Problem:** `_interfaces/GUI/library/makefile_manager.py` used three `dirname` calls, resolving to `_interfaces/` instead of `CoolBox/`.

The file lives at `_interfaces/GUI/library/makefile_manager.py`, so four levels are needed:
`library/` → `GUI/` → `_interfaces/` → `CoolBox/`

**Fix:** Updated to four `dirname` calls.

---

## 3. Stale `_libraries/apps/_Product` scan paths

**Problem:** Several scan endpoints in `main.py` used old paths that no longer existed after the deliverables reorganisation:

| Old path | New path |
|---|---|
| `_libraries/` | `_deliverables/libraries/groups/` |
| `apps/` | `_deliverables/apps/` |
| `_Product/` | `_deliverables/Product/` |

**Fix:** All scan paths updated to the `_deliverables/` layout.

---

## 4. Client FE and Middleware paths

**Problem:** `GET /client-fe` and `GET /middle-wear` pointed to `business_suite/client_fe` and `business_suite/middle_wear` at the repo root, which do not exist. The actual location is `_interfaces/business_suite/`.

**Fix:**

```python
client_fe_dir = os.path.join(repo_root, "_interfaces", "business_suite", "client_fe")
mw_dir        = os.path.join(repo_root, "_interfaces", "business_suite", "middle_wear")
```

---

## 5. Package Builder sub-tab layout

Changes to `package-builder.mjs`:

- **Middleware** removed as a standalone top-level tab; embedded inside **Client FE** as an inner sub-tab (`🌐 Portals` / `🔧 Middleware`).
- **Extensions** tab added as a new top-level tab (`🧩 Extensions`), backed by `<extension-builder>`.
- Tab icon `✦` position: placed next to **Modern** tab label (`Modern ✦`).
