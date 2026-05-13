import mimetypes
mimetypes.add_type("application/javascript", ".mjs")
mimetypes.add_type("application/javascript", ".js")

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "library"))
import makefile_manager as mm

from fastapi import FastAPI, Request
from fastapi.responses import FileResponse, JSONResponse, StreamingResponse
from sqlmodel import SQLModel, Field, Session, create_engine, select
from starlette.staticfiles import StaticFiles
from starlette.responses import FileResponse as StarletteFileResponse
from typing import Optional, List
import asyncio
import getpass
import platform
import subprocess
import re


class NoCacheStaticFiles(StaticFiles):
    async def get_response(self, path, scope):
        response = await super().get_response(path, scope)
        response.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
        response.headers["Pragma"] = "no-cache"
        response.headers["Expires"] = "0"
        return response


app = FastAPI()
app.mount("/static", NoCacheStaticFiles(directory="static"), name="static")


class Item(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    name: str
    description: Optional[str] = None


engine = create_engine("sqlite:///./dashboard.db", echo=True)


@app.on_event("startup")
def on_startup():
    SQLModel.metadata.create_all(engine)


@app.post("/items/", response_model=Item)
def create_item(item: Item):
    with Session(engine) as session:
        session.add(item)
        session.commit()
        session.refresh(item)
        return item


@app.get("/items/", response_model=List[Item])
def read_items():
    with Session(engine) as session:
        items = session.exec(select(Item)).all()
        return items


@app.get("/")
def serve_dashboard():
    return FileResponse("static/index.html")


def scan_groups() -> list:
    """Dynamically scan _deliverables/libraries/groups for all groups and their subpackages."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    groups_path = os.path.join(repo_root, "_deliverables", "libraries", "groups")
    groups = []
    if not os.path.isdir(groups_path):
        return groups
    for group_name in sorted(os.listdir(groups_path)):
        group_dir = os.path.join(groups_path, group_name)
        if not os.path.isdir(group_dir):
            continue
        libs = sorted(
            entry for entry in os.listdir(group_dir)
            if os.path.isdir(os.path.join(group_dir, entry)) and entry != "__pycache__"
        )
        groups.append({"name": group_name, "libs": libs})
    return groups


@app.get("/dashboard")
def dashboard(request: Request):
    groups = scan_groups()
    tests = mm.scan_tests()
    server_url = str(request.base_url).rstrip("/")
    user = getpass.getuser()
    try:
        branch = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=os.path.dirname(os.path.abspath(__file__)),
            text=True
        ).strip()
    except Exception:
        branch = "unknown"
    return JSONResponse({
        "groups": groups,
        "tests": tests,
        "server_url": server_url,
        "user": user,
        "branch": branch,
    })


@app.post("/build")
async def build_library(request: Request):
    """Build a library target via Makefile (build_<lib> rule)."""
    body = await request.json()
    group = body.get("group", "").strip()
    lib = body.get("lib", "").strip()
    if not re.match(r'^[\w\-]+$', group) or not re.match(r'^[\w\-]+$', lib):
        return JSONResponse({"success": False, "output": "Invalid group or lib name."}, status_code=400)
    result = mm.build_target(lib, timeout=300)
    return JSONResponse(result)


@app.get("/download")
async def download_artifact(path: str):
    """Serve a build artifact for download.
    `path` must be a repo-relative path under the build/ directory."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    # Resolve and verify the path stays inside build/
    try:
        target = os.path.realpath(os.path.join(repo_root, path))
        build_root = os.path.realpath(os.path.join(repo_root, "build"))
    except Exception:
        return JSONResponse({"error": "Invalid path."}, status_code=400)
    if not target.startswith(build_root + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(target):
        return JSONResponse({"error": "File not found."}, status_code=404)
    return FileResponse(target, filename=os.path.basename(target))


@app.post("/test/build")
async def test_build(request: Request):
    """Build a test target via Makefile.
    Prefers the scanned cmake build_target (vcxproj target name); falls back to ctest name."""
    body = await request.json()
    test_name = body.get("name", "").strip()
    build_target = body.get("build_target", "").strip()
    if not re.match(r'^[\w:\.\- ]+$', test_name):
        return JSONResponse({"success": False, "output": "Invalid test name."}, status_code=400)
    # Use the actual cmake target name if we found one; else use the ctest name directly
    cmake_target = build_target if build_target and re.match(r'^[\w\-]+$', build_target) else test_name
    result = mm.build_target(cmake_target, timeout=300)
    return JSONResponse(result)


@app.post("/test/run")
async def test_run(request: Request):
    """Run a CTest test by its ctest name directly (no build step)."""
    body = await request.json()
    test_name = body.get("name", "").strip()
    if not re.match(r'^[\w:\.\-]+$', test_name):
        return JSONResponse({"success": False, "output": "Invalid test name."}, status_code=400)
    result = mm.ctest_run(test_name, timeout=180)
    return JSONResponse(result)


@app.get("/deps/{target}")
def get_deps(target: str):
    """Return cmake --graphviz dependency info for the given CMake target."""
    if not re.match(r'^[\w\-\.]+$', target):
        return JSONResponse({"success": False, "edges": [], "dot": None,
                             "output": "Invalid target.", "available_targets": []}, status_code=400)
    return JSONResponse(mm.get_deps(target))


def _parse_cmake_target(cmake_file: str) -> str:
    """Parse CMakeLists.txt and return the first add_library/add_executable target name."""
    if not os.path.isfile(cmake_file):
        return ""
    try:
        with open(cmake_file, encoding='utf-8', errors='replace') as fh:
            content = fh.read()
        m = re.search(r'add_(?:library|executable)\s*\(\s*(\w+)', content)
        return m.group(1) if m else ""
    except Exception:
        return ""


def _scan_lib_dir(dir_path: str, repo_root: str) -> list:
    """Recursively walk a group/package directory collecting leaf libraries.
    A leaf is a directory that contains a 'headers/' subfolder.
    Returns a list of dicts: {name, cmake_target, headers: [{name, path}]}."""
    results = []
    headers_dir = os.path.join(dir_path, "headers")
    if os.path.isdir(headers_dir):
        # Leaf library â€” gather header files
        headers = []
        for fname in sorted(os.listdir(headers_dir)):
            fpath = os.path.join(headers_dir, fname)
            if os.path.isfile(fpath) and fname.split('.')[-1].lower() in ('h', 'hpp', 'hxx'):
                rel = os.path.relpath(fpath, repo_root).replace('\\', '/')
                headers.append({"name": fname, "path": rel})
        cmake_target = _parse_cmake_target(os.path.join(dir_path, "CMakeLists.txt"))
        results.append({
            "name": os.path.basename(dir_path),
            "cmake_target": cmake_target,
            "headers": headers,
        })
    else:
        # Package directory â€” recurse into children
        try:
            entries = sorted(os.listdir(dir_path))
        except PermissionError:
            return results
        for entry in entries:
            if entry.startswith(('_', '.')):
                continue
            child = os.path.join(dir_path, entry)
            if os.path.isdir(child):
                results.extend(_scan_lib_dir(child, repo_root))
    return results


@app.get("/library/info")
def library_info(group: str = "", lib: str = ""):
    """Return individual sub-libraries and their header files for a given group+lib.
    Response: {group, lib, libs: [{name, cmake_target, headers: [{name, path}]}]}"""
    if not re.match(r'^[\w\-]+$', group) or not re.match(r'^[\w\-]+$', lib):
        return JSONResponse({"error": "Invalid group or lib name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base_dir = os.path.realpath(os.path.join(repo_root, "_deliverables", "libraries", "groups", group, lib))
    allowed = os.path.realpath(os.path.join(repo_root, "_deliverables", "libraries", "groups"))
    if not base_dir.startswith(allowed + os.sep) and base_dir != allowed:
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isdir(base_dir):
        return JSONResponse({"error": f"'{group}/{lib}' not found."}, status_code=404)
    libs = _scan_lib_dir(base_dir, repo_root)
    return JSONResponse({"group": group, "lib": lib, "libs": libs})


@app.get("/library/file")
def library_file(path: str = ""):
    """Return the text content of a header file within _libraries/.
    'path' must be a repo-relative path (forward slashes)."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if not path:
        return JSONResponse({"error": "path is required."}, status_code=400)
    # Normalise and security-check
    full = os.path.realpath(os.path.join(repo_root, path.replace('/', os.sep)))
    allowed = os.path.realpath(os.path.join(repo_root, "_deliverables", "libraries"))
    if not full.startswith(allowed + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(full):
        return JSONResponse({"error": "File not found."}, status_code=404)
    try:
        with open(full, encoding='utf-8', errors='replace') as fh:
            content = fh.read()
        return JSONResponse({"path": path, "name": os.path.basename(full), "content": content})
    except Exception as e:
        return JSONResponse({"error": str(e)}, status_code=500)


def _scan_products() -> list:
    """Scan _deliverables/Product/ subdirectories and parse cmake exe targets from CMakeLists.txt."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    product_dir = os.path.join(repo_root, "_deliverables", "Product")
    products = []
    if not os.path.isdir(product_dir):
        return products
    for entry in sorted(os.listdir(product_dir)):
        sub = os.path.join(product_dir, entry)
        if not os.path.isdir(sub) or entry.startswith(('_', '.')):
            continue
        cmake = os.path.join(sub, "CMakeLists.txt")
        if not os.path.isfile(cmake):
            continue
        try:
            with open(cmake, encoding='utf-8', errors='replace') as fh:
                content = fh.read()
        except Exception:
            content = ""
        executables = re.findall(r'add_executable\s*\(\s*(\w+)', content)
        has_page = os.path.isfile(os.path.join(sub, "product_page.html"))
        products.append({"name": entry, "folder": entry, "executables": executables,
                         "has_page": has_page})
    return products


def _find_product_exe(exe_name: str, build_dir: str) -> str:
    """Find a built product exe in build/. Returns absolute path or empty string."""
    exe_variants = [exe_name, exe_name + ".exe"]
    for dirpath, dirs, files in os.walk(build_dir):
        dirs[:] = [d for d in dirs if d not in {'CMakeFiles', '.cmake'}]
        for fname in files:
            if fname in exe_variants:
                return os.path.join(dirpath, fname)
    return ""


@app.get("/products")
def get_products():
    """Return a list of buildable/launchable products from _Product/."""
    return JSONResponse({"products": _scan_products()})


@app.post("/products/build")
async def build_product(request: Request):
    """Build a product exe via cmake --build. Body: {name, target}"""
    body = await request.json()
    name = body.get("name", "").strip()
    target = body.get("target", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', target):
        return JSONResponse({"success": False, "output": "Invalid name or target."}, status_code=400)
    result = mm.build_target(target, timeout=300)
    return JSONResponse(result)


@app.post("/products/launch")
async def launch_product(request: Request):
    """Launch a built product exe as a detached process. Body: {name, exe}"""
    body = await request.json()
    name = body.get("name", "").strip()
    exe_name = body.get("exe", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', exe_name):
        return JSONResponse({"success": False, "output": "Invalid name or exe."}, status_code=400)

    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    build_dir = os.path.join(repo_root, "build")
    exe_path = _find_product_exe(exe_name, build_dir)
    if not exe_path:
        return JSONResponse({"success": False,
                             "output": f"'{exe_name}' not found in build/. Build it first."})
    try:
        if platform.system() == "Windows":
            import ctypes
            DETACHED_PROCESS = 0x00000008
            CREATE_NEW_PROCESS_GROUP = 0x00000200
            subprocess.Popen(
                [exe_path],
                cwd=os.path.dirname(exe_path),
                creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                close_fds=True,
            )
        else:
            subprocess.Popen(
                [exe_path],
                cwd=os.path.dirname(exe_path),
                start_new_session=True,
                close_fds=True,
            )
        return JSONResponse({"success": True, "output": f"Launched {exe_name}."})
    except Exception as e:
        return JSONResponse({"success": False, "output": str(e)})


@app.get("/product-page/{folder}")
def product_page(folder: str):
    """Serve the product_page.html for a product in _Product/{folder}/."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "_deliverables", "Product"))
    page = os.path.realpath(os.path.join(base, folder, "product_page.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "product_page.html not found for this product."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


# â”€â”€â”€ Apps endpoints â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def _scan_apps() -> list:
    """Scan _deliverables/apps/ subdirectories. Returns list of app descriptors."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    apps_dir = os.path.join(repo_root, "_deliverables", "apps")
    apps = []
    if not os.path.isdir(apps_dir):
        return apps
    for entry in sorted(os.listdir(apps_dir)):
        sub = os.path.join(apps_dir, entry)
        if not os.path.isdir(sub) or entry.startswith(('_', '.')):
            continue
        # Determine type and targets
        cmake = os.path.join(sub, "CMakeLists.txt")
        executables: list = []
        scripts: list = []
        app_type = "other"
        if os.path.isfile(cmake):
            app_type = "cmake"
            try:
                with open(cmake, encoding='utf-8', errors='replace') as fh:
                    content = fh.read()
                executables = re.findall(r'add_executable\s*\(\s*(\w+)', content)
            except Exception:
                pass
        # Detect Python scripts in the root of the folder
        for fname in os.listdir(sub):
            if fname.endswith('.py') and os.path.isfile(os.path.join(sub, fname)):
                scripts.append(fname)
                if app_type == "other":
                    app_type = "python"
        has_page = os.path.isfile(os.path.join(sub, "app_page.html"))
        apps.append({
            "name": entry,
            "folder": entry,
            "type": app_type,
            "executables": executables,
            "scripts": scripts,
            "has_page": has_page,
        })
    return apps


@app.get("/apps")
def get_apps():
    """Return a list of apps from _deliverables/apps/."""
    return JSONResponse({"apps": _scan_apps()})


@app.post("/apps/build")
async def build_app(request: Request):
    """Build an app cmake target. Body: {name, target}"""
    body = await request.json()
    name = body.get("name", "").strip()
    target = body.get("target", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', target):
        return JSONResponse({"success": False, "output": "Invalid name or target."}, status_code=400)
    result = mm.build_target(target, timeout=300)
    return JSONResponse(result)


@app.post("/apps/launch")
async def launch_app(request: Request):
    """Launch a built app exe as a detached process. Body: {name, exe}"""
    body = await request.json()
    name = body.get("name", "").strip()
    exe_name = body.get("exe", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', exe_name):
        return JSONResponse({"success": False, "output": "Invalid name or exe."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    build_dir = os.path.join(repo_root, "build")
    exe_path = _find_product_exe(exe_name, build_dir)
    if not exe_path:
        return JSONResponse({"success": False,
                             "output": f"'{exe_name}' not found in build/. Build it first."})
    try:
        if platform.system() == "Windows":
            import ctypes
            DETACHED_PROCESS = 0x00000008
            CREATE_NEW_PROCESS_GROUP = 0x00000200
            subprocess.Popen(
                [exe_path],
                cwd=os.path.dirname(exe_path),
                creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                close_fds=True,
            )
        else:
            subprocess.Popen(
                [exe_path],
                cwd=os.path.dirname(exe_path),
                start_new_session=True,
                close_fds=True,
            )
        return JSONResponse({"success": True, "output": f"Launched {exe_name}."})
    except Exception as e:
        return JSONResponse({"success": False, "output": str(e)})


@app.post("/apps/run-script")
async def run_app_script(request: Request):
    """Run a Python script from an app folder. Body: {name, script}"""
    body = await request.json()
    name = body.get("name", "").strip()
    script = body.get("script", "").strip()
    # Validate: folder name and script name (no path traversal)
    if not re.match(r'^[\w\-]+$', name):
        return JSONResponse({"success": False, "output": "Invalid app name."}, status_code=400)
    if not re.match(r'^[\w\-]+\.py$', script):
        return JSONResponse({"success": False, "output": "Invalid script name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    apps_dir = os.path.join(repo_root, "_deliverables", "apps")
    script_path = os.path.realpath(os.path.join(apps_dir, name, script))
    # Path traversal guard
    if not script_path.startswith(os.path.realpath(apps_dir) + os.sep):
        return JSONResponse({"success": False, "output": "Access denied."}, status_code=403)
    if not os.path.isfile(script_path):
        return JSONResponse({"success": False, "output": f"Script '{script}' not found."}, status_code=404)
    try:
        python_exe = sys.executable
        if platform.system() == "Windows":
            DETACHED_PROCESS = 0x00000008
            CREATE_NEW_PROCESS_GROUP = 0x00000200
            subprocess.Popen(
                [python_exe, script_path],
                cwd=os.path.dirname(script_path),
                creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                close_fds=True,
            )
        else:
            subprocess.Popen(
                [python_exe, script_path],
                cwd=os.path.dirname(script_path),
                start_new_session=True,
                close_fds=True,
            )
        return JSONResponse({"success": True, "output": f"Launched {script}."})
    except Exception as e:
        return JSONResponse({"success": False, "output": str(e)})


@app.get("/app-page/{folder}")
def app_page(folder: str):
    """Serve the app_page.html for an app in _deliverables/apps/{folder}/."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "_deliverables", "apps"))
    page = os.path.realpath(os.path.join(base, folder, "app_page.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "app_page.html not found for this app."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


@app.get("/extensions")
def list_extensions():
    """List all binding packages from _deliverables/libraries/bindings/."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    bindings_dir = os.path.join(repo_root, "_deliverables", "libraries", "bindings")
    if not os.path.isdir(bindings_dir):
        return JSONResponse({"bindings": []})

    # Map binding folder name → language label
    _LANG_MAP = {
        "c_bindings":          "C",
        "c3_bindings":         "C3",
        "emscripten_bindings": "Emscripten",
        "go_bindings":         "Go",
        "java_bindings":       "Java",
        "python_bindings":     "Python",
        "r_bindings":          "R",
        "rust_bindings":       "Rust",
        "vlang_bindings":      "V",
    }

    bindings = []
    for entry in sorted(os.listdir(bindings_dir)):
        entry_path = os.path.join(bindings_dir, entry)
        if not os.path.isdir(entry_path) or entry.startswith(('.', '_')):
            continue
        has_cmake   = os.path.isfile(os.path.join(entry_path, "CMakeLists.txt"))
        has_setup   = (os.path.isfile(os.path.join(entry_path, "setup.py")) or
                       os.path.isfile(os.path.join(entry_path, "pyproject.toml")))
        has_cargo   = os.path.isfile(os.path.join(entry_path, "Cargo.toml"))
        has_go      = os.path.isfile(os.path.join(entry_path, "go.mod"))
        has_pom     = os.path.isfile(os.path.join(entry_path, "pom.xml"))
        has_package = os.path.isfile(os.path.join(entry_path, "package.json"))
        has_v_mod   = (os.path.isfile(os.path.join(entry_path, "v.mod")) or
                       bool(list(__import__("glob").glob(os.path.join(entry_path, "*.v")))))
        has_c3      = bool(list(__import__("glob").glob(os.path.join(entry_path, "**", "*.c3"), recursive=True)))
        has_r       = os.path.isfile(os.path.join(entry_path, "DESCRIPTION"))

        # Emscripten needs its own cmake+emcc toolchain build
        if entry == "emscripten_bindings":
            import shutil as _shutil
            repo_root_emcc = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            _emcc_managed = os.path.join(
                repo_root_emcc, "_local_build_pipeline", "tmp", "installers",
                "emsdk", "upstream", "emscripten",
                "emcc.bat" if os.name == "nt" else "emcc"
            )
            build_type = "emscripten"
            has_emcc   = bool(
                _shutil.which("emcc") or
                os.path.isfile(_emcc_managed) or
                os.path.isfile(r"C:\emsdk\upstream\emscripten\emcc.bat")
            )
        else:
            # Python: prefer pip over cmake even when CMakeLists.txt is present
            build_type = (
                "emscripten" if entry == "emscripten_bindings" else
                "python"     if has_setup   else
                "cmake"      if has_cmake   else
                "cargo"      if has_cargo   else
                "go"         if has_go      else
                "maven"      if has_pom     else
                "npm"        if has_package else
                "vlang"      if has_v_mod   else
                "c3"         if has_c3      else
                "r"          if has_r       else
                "unknown"
            )
            has_emcc = None

        # Go: check if go is actually installed (including managed install dir)
        has_go_exec = None
        if has_go:
            import shutil as _shutil
            import glob as _glob
            _repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            _go_managed = os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                                       "go_install", "go", "bin",
                                       "go.exe" if os.name == "nt" else "go")
            has_go_exec = bool(
                _shutil.which("go") or
                os.path.isfile(_go_managed) or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\Go\bin\go.exe",
                    r"C:\Go\bin\go.exe",
                ])
            )

        # Maven: check if mvn is installed (including managed install dir)
        has_mvn_exec = None
        has_java_exec = None
        if has_pom:
            import shutil as _shutil
            import glob as _glob
            _repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            _mvn_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "maven_install", "apache-maven-*", "bin",
                             "mvn.cmd" if os.name == "nt" else "mvn"),
            )
            has_mvn_exec = bool(
                _shutil.which("mvn") or _shutil.which("mvn.cmd") or
                _mvn_managed or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\Maven\bin\mvn.cmd",
                    r"C:\tools\maven\bin\mvn.cmd",
                    r"C:\mvn\bin\mvn.cmd",
                ])
            )
            # Java (JDK) is required by Maven
            _java_exe = "java.exe" if os.name == "nt" else "java"
            _jdk_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "jdk_install", "jdk-*", "bin", _java_exe),
            )
            _jdk_system = []
            if os.name == "nt":
                import glob as _g
                for _pat in [
                    r"C:\Program Files\Java\jdk-*\bin\java.exe",
                    r"C:\Program Files\Eclipse Adoptium\jdk-*\bin\java.exe",
                    r"C:\Program Files\Microsoft\jdk-*\bin\java.exe",
                ]:
                    _jdk_system.extend(_g.glob(_pat))
            has_java_exec = bool(
                _shutil.which("java") or _jdk_managed or _jdk_system or
                os.environ.get("JAVA_HOME")
            )

        # V compiler: check if v is installed (including managed install dir)
        has_v_exec = None
        if has_v_mod:
            import shutil as _shutil
            import glob as _glob
            _repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            _v_exe = "v.exe" if os.name == "nt" else "v"
            _v_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "vlang_install", "**", _v_exe), recursive=True)
            has_v_exec = bool(
                _shutil.which("v") or
                any(os.path.isfile(p) for p in [
                    r"C:\V\v.exe",
                    r"C:\tools\vlang\v.exe",
                ]) or
                _v_managed
            )

        # C3 compiler: check if c3c is installed (including managed install dir)
        has_c3c_exec = None
        if has_c3:
            import shutil as _shutil
            import glob as _glob
            _repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            _c3c_exe = "c3c.exe" if os.name == "nt" else "c3c"
            _c3c_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "c3c_install", "**", _c3c_exe), recursive=True)
            has_c3c_exec = bool(
                _shutil.which("c3c") or
                any(os.path.isfile(p) for p in [
                    r"C:\c3\c3c.exe",
                    r"C:\tools\c3\c3c.exe",
                ]) or
                _c3c_managed
            )

        # R: check if R/Rscript is installed
        has_r_exec = None
        has_rtools = None
        if has_r:
            import shutil as _shutil
            has_r_exec = bool(
                _shutil.which("Rscript") or _shutil.which("R") or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\R\R-4.6.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.5.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.2.0\bin\Rscript.exe",
                ])
            )
            # On Windows, R packages with C++ code also need Rtools (gcc)
            if os.name == "nt":
                has_rtools = any(os.path.isfile(p) for p in [
                    r"C:\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools44\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools43\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools42\mingw64\bin\gcc.exe",
                    os.path.expanduser(r"~\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe"),
                ])
            else:
                has_rtools = True  # Linux/macOS use system gcc
        # Parse CMakeLists.txt to find the real primary cmake target name
        cmake_target = None
        if has_cmake:
            try:
                with open(os.path.join(entry_path, "CMakeLists.txt"), encoding="utf-8", errors="replace") as f:
                    cml = f.read()
                import re as _re
                m = _re.search(r'add_library\s*\(\s*([\w]+)\s+(?!ALIAS)', cml, _re.IGNORECASE)
                if not m:
                    m = _re.search(r'add_executable\s*\(\s*([\w]+)', cml, _re.IGNORECASE)
                if not m:
                    m = _re.search(r'pybind11_add_module\s*\(\s*([\w]+)', cml, _re.IGNORECASE)
                if m:
                    cmake_target = m.group(1)
            except Exception:
                pass
        bindings.append({
            "name":         entry,
            "lang":         _LANG_MAP.get(entry, entry.replace("_bindings", "").title()),
            "build_type":   build_type,
            "cmake_target": cmake_target,
            "has_cmake":    has_cmake,
            "has_cargo":    has_cargo,
            "has_go":       has_go,
            "has_pom":      has_pom,
            "has_setup":    has_setup,
            "has_emcc":     has_emcc,
            "has_go_exec":  has_go_exec,
            "has_mvn_exec": has_mvn_exec,
            "has_java_exec": has_java_exec,
            "has_v_exec":   has_v_exec,
            "has_c3c_exec": has_c3c_exec,
            "has_r_exec":   has_r_exec,
            "has_rtools":   has_rtools,
        })
    return JSONResponse({"bindings": bindings})


@app.get("/extensions/tools")
def list_tools():
    """Return install-status for every toolchain supported by master_installer.py."""
    import shutil as _shutil
    import glob as _glob
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    _tmp = os.path.join(repo_root, "_local_build_pipeline", "tmp", "installers")
    _ext = ".exe" if os.name == "nt" else ""
    _bat = ".bat" if os.name == "nt" else ""

    def _managed(*rel):
        """Return a path inside the managed installers dir."""
        return os.path.join(_tmp, *rel)

    def _glob_managed(*pattern):
        """Return True if any file matching glob exists inside managed installers dir."""
        return bool(_glob.glob(os.path.join(_tmp, *pattern), recursive=True))

    # Managed Go path is deterministic
    _go_managed = _managed("go_install", "go", "bin", f"go{_ext}")
    # Managed Maven: version varies, glob for mvn.cmd / mvn
    _mvn_pattern = ("maven_install", "apache-maven-*", "bin", f"mvn{_bat or _ext}")
    # Managed jdk: version dir varies, glob for java.exe
    _jdk_pattern = ("jdk_install", "jdk-*", "bin", f"java{_ext}")
    # Managed c3c: extracted dir varies, recurse
    _c3c_pattern = ("c3c_install", "**", f"c3c{_ext}")
    # Managed vlang: extracted dir varies, recurse
    _v_pattern = ("vlang_install", "**", f"v{_ext}")
    # Managed emsdk
    _emsdk_managed = _managed("emsdk")
    _emcc_managed = _managed("emsdk", "upstream", "emscripten", f"emcc{_bat}")

    TOOLS = {
        "go":    {"label": "Go compiler",       "check": ["go"],
                  "extra": [_go_managed,
                             r"C:\Program Files\Go\bin\go.exe", r"C:\Go\bin\go.exe"]},
        "maven": {"label": "Apache Maven",      "check": ["mvn", "mvn.cmd"],
                  "extra": [r"C:\Program Files\Maven\bin\mvn.cmd", r"C:\tools\maven\bin\mvn.cmd"],
                  "glob":  _mvn_pattern},
        "jdk":   {"label": "JDK 21 (Temurin)",  "check": ["java"],
                  "extra": [r"C:\Program Files\Java\bin\java.exe",
                             r"C:\Program Files\Eclipse Adoptium\bin\java.exe"],
                  "glob":  _jdk_pattern},
        "c3c":   {"label": "C3 compiler",       "check": ["c3c"],
                  "extra": [r"C:\c3\c3c.exe", r"C:\tools\c3\c3c.exe"],
                  "glob":  _c3c_pattern},
        "vlang": {"label": "V compiler",        "check": ["v"],
                  "extra": [r"C:\V\v.exe", r"C:\tools\vlang\v.exe"],
                  "glob":  _v_pattern},
        "r":     {"label": "R / Rscript",       "check": ["Rscript", "R"],
                  "extra": [r"C:\Program Files\R\R-4.6.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.5.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe"]},
        "rtools": {"label": "Rtools (gcc for R)", "check": [],
                   "extra": [
                       r"C:\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools44\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools43\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools42\mingw64\bin\gcc.exe",
                       os.path.expanduser(r"~\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe"),
                   ]},
        "emsdk": {"label": "Emscripten (emcc)", "check": ["emcc"],
                  "extra": [_emcc_managed,
                             r"C:\emsdk\upstream\emscripten\emcc.bat"]},
    }
    result = []
    for key, meta in TOOLS.items():
        installed = (
            any(_shutil.which(n) for n in meta["check"]) or
            any(os.path.isfile(p) for p in meta.get("extra", [])) or
            ("glob" in meta and _glob_managed(*meta["glob"]))
        )
        result.append({"tool": key, "label": meta["label"], "installed": installed})
    return JSONResponse({"tools": result})


@app.post("/extensions/install")
async def install_tool(request: Request):
    """Stream master_installer.py output for the requested tool.
    Body: { tool: str }  e.g. "go" | "maven" | "c3c" | "vlang" | "r" | "all"
    Response: text/plain stream, last line is __EXIT_CODE__:<n>
    """
    body = await request.json()
    tool = body.get("tool", "").strip()
    VALID_TOOLS = {"go", "maven", "jdk", "c3c", "vlang", "r", "rtools", "emsdk", "all"}
    if tool not in VALID_TOOLS:
        return JSONResponse({"success": False, "output": f"Unknown tool: {tool}"}, status_code=400)

    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    script = os.path.join(repo_root, "_scripts", "install_scripts", "master_installer.py")
    if not os.path.isfile(script):
        return JSONResponse({"success": False, "output": "master_installer.py not found."}, status_code=500)

    cmd = [sys.executable, "-u", script, tool if tool != "all" else "--all"]

    async def _stream():
        loop = asyncio.get_event_loop()
        queue: asyncio.Queue = asyncio.Queue()

        def _run_proc():
            try:
                proc = subprocess.Popen(
                    cmd, cwd=repo_root,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding="utf-8", errors="replace",
                )
                for line in proc.stdout:
                    loop.call_soon_threadsafe(queue.put_nowait, line)
                proc.wait()
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\n__EXIT_CODE__:{proc.returncode}\n"
                )
            except Exception as exc:
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\nERROR: {exc}\n__EXIT_CODE__:1\n"
                )
            finally:
                loop.call_soon_threadsafe(queue.put_nowait, None)

        loop.run_in_executor(None, _run_proc)

        while True:
            item = await queue.get()
            if item is None:
                break
            yield item

    return StreamingResponse(_stream(), media_type="text/plain")


@app.post("/extensions/build")
async def build_extension(request: Request):
    """Stream _scripts/build_scripts/build_extensions.py output for the given binding.
    Body: { binding: str }  -- binding folder name, e.g. "python_bindings"
          { binding: "" }   -- empty string means build all
    Response: text/plain stream, last line is __EXIT_CODE__:<n>
    """
    body = await request.json()
    binding = body.get("binding", "").strip()
    if binding and not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"success": False, "output": "Invalid binding name."}, status_code=400)

    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    script = os.path.join(repo_root, "_scripts", "build_scripts", "build_extensions.py")
    if not os.path.isfile(script):
        return JSONResponse({"success": False, "output": "build_extensions.py not found."}, status_code=500)

    cmd = [sys.executable, "-u", script]
    if binding:
        cmd.append(binding)

    async def _stream():
        # asyncio.create_subprocess_exec requires ProactorEventLoop on Windows,
        # which uvicorn doesn't use. Run the blocking Popen in a thread pool and
        # forward lines via an asyncio.Queue with call_soon_threadsafe.
        loop = asyncio.get_event_loop()
        queue: asyncio.Queue = asyncio.Queue()

        def _run_proc():
            try:
                proc = subprocess.Popen(
                    cmd, cwd=repo_root,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding='utf-8', errors='replace',
                )
                for line in proc.stdout:
                    loop.call_soon_threadsafe(queue.put_nowait, line)
                proc.wait()
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\n__EXIT_CODE__:{proc.returncode}\n"
                )
            except Exception as exc:
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\nERROR: {exc}\n__EXIT_CODE__:1\n"
                )
            finally:
                loop.call_soon_threadsafe(queue.put_nowait, None)  # sentinel

        loop.run_in_executor(None, _run_proc)

        while True:
            item = await queue.get()
            if item is None:
                break
            yield item

    return StreamingResponse(_stream(), media_type="text/plain")


@app.get("/extensions/artifacts")
def list_extension_artifacts(binding: str):
    """Return downloadable artifacts produced by a binding build.
    Scans the cmake Debug output dir and the binding's own target/ dir.
    Query param: binding=<folder_name>  e.g. c_bindings
    """
    if not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"error": "Invalid binding name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    ARTIFACT_EXTS = {".dll", ".so", ".dylib", ".wasm", ".js", ".a", ".lib",
                     ".jar", ".pyd", ".exe", ".rlib"}

    artifacts = []
    seen: set = set()

    def _scan(directory: str):
        if not os.path.isdir(directory):
            return
        for fname in sorted(os.listdir(directory)):
            _, ext = os.path.splitext(fname)
            if ext.lower() in ARTIFACT_EXTS and fname not in seen:
                fpath = os.path.join(directory, fname)
                if os.path.isfile(fpath):
                    seen.add(fname)
                    artifacts.append({
                        "name": fname,
                        "size": os.path.getsize(fpath),
                        "url":  f"/extensions/download?binding={binding}&file={fname}",
                    })

    # cmake builds land in build/_deliverables/libraries/bindings/<name>/Debug/
    _scan(os.path.join(repo_root, "build", "_deliverables", "libraries", "bindings", binding, "Debug"))
    # cargo builds output into target/debug/
    _scan(os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding, "target", "debug"))
    _scan(os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding, "target"))
    # Python pip installs place .pyd files in the binding root and its immediate subdirs
    binding_root = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    _scan(binding_root)
    if os.path.isdir(binding_root):
        for entry in sorted(os.listdir(binding_root)):
            sub = os.path.join(binding_root, entry)
            if os.path.isdir(sub):
                _scan(sub)

    return JSONResponse({"binding": binding, "artifacts": artifacts})


@app.get("/extensions/download")
def download_extension_artifact(binding: str, file: str):
    """Serve a build artifact file for download."""
    if not re.match(r'^[\w\-]+$', binding) or not re.match(r'^[\w\-\.]+$', file):
        return JSONResponse({"error": "Invalid parameters."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    ARTIFACT_EXTS = {".dll", ".so", ".dylib", ".wasm", ".js", ".a", ".lib",
                     ".jar", ".pyd", ".exe", ".rlib"}
    _, ext = os.path.splitext(file)
    if ext.lower() not in ARTIFACT_EXTS:
        return JSONResponse({"error": "File type not allowed."}, status_code=403)

    # Build search dirs: same set as list_extension_artifacts
    binding_root = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    search_dirs = [
        os.path.join(repo_root, "build", "_deliverables", "libraries", "bindings", binding, "Debug"),
        os.path.join(binding_root, "target", "debug"),
        os.path.join(binding_root, "target"),
        binding_root,
    ]
    # Add one level of subdirectories under binding root (e.g. ml_toolbox/ for Python)
    if os.path.isdir(binding_root):
        for entry in sorted(os.listdir(binding_root)):
            sub = os.path.join(binding_root, entry)
            if os.path.isdir(sub):
                search_dirs.append(sub)

    for directory in search_dirs:
        if not os.path.isdir(directory):
            continue
        base_real = os.path.realpath(directory)
        candidate = os.path.realpath(os.path.join(directory, file))
        if candidate.startswith(base_real + os.sep) and os.path.isfile(candidate):
            return StarletteFileResponse(
                candidate,
                filename=file,
                media_type="application/octet-stream",
            )

    return JSONResponse({"error": "Artifact not found."}, status_code=404)


@app.get("/extensions/docs")
def get_extension_docs(binding: str):
    """Return README.md content for a binding if available."""
    if not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"error": "Invalid binding name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    binding_dir = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    if not os.path.isdir(binding_dir):
        return JSONResponse({"content": "", "filename": None})
    for doc_name in ("README.md", "README.txt", "DOCS.md"):
        doc_path = os.path.join(binding_dir, doc_name)
        if os.path.isfile(doc_path):
            with open(doc_path, "r", encoding="utf-8") as f:
                return JSONResponse({"content": f.read(), "filename": doc_name})
    return JSONResponse({"content": "", "filename": None})


@app.get("/client-fe")
def list_client_fe():
    """List all client portals inside _interfaces/business_suite/client_fe/."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    client_fe_dir = os.path.join(repo_root, "_interfaces", "business_suite", "client_fe")
    if not os.path.isdir(client_fe_dir):
        return JSONResponse({"portals": []})
    portals = []
    for entry in sorted(os.listdir(client_fe_dir)):
        portal_path = os.path.join(client_fe_dir, entry)
        if not os.path.isdir(portal_path):
            continue
        has_index = os.path.isfile(os.path.join(portal_path, "index.html"))
        # Read optional metadata from a portal.json if present
        meta_path = os.path.join(portal_path, "portal.json")
        title = None
        description = None
        if os.path.isfile(meta_path):
            try:
                import json as _json
                with open(meta_path, "r", encoding="utf-8") as f:
                    meta = _json.load(f)
                title = meta.get("title")
                description = meta.get("description")
            except Exception:
                pass
        portals.append({
            "folder":      entry,
            "title":       title,
            "description": description,
            "has_index":   has_index,
        })
    return JSONResponse({"portals": portals})


# â”€â”€â”€ Plans endpoints â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def _scan_plans() -> dict:
    """Recursively scan plan/ and return a JSON tree of folders and .md files."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    plan_dir = os.path.join(repo_root, "_internal_documents", "plan")

    def walk(path: str, rel: str) -> dict:
        node = {"name": os.path.basename(path), "path": rel, "type": "dir", "children": []}
        try:
            entries = sorted(os.listdir(path))
        except PermissionError:
            return node
        for entry in entries:
            if entry.startswith('.'):
                continue
            full = os.path.join(path, entry)
            child_rel = f"{rel}/{entry}" if rel else entry
            if os.path.isdir(full):
                node["children"].append(walk(full, child_rel))
            elif entry.lower().endswith('.md'):
                node["children"].append({"name": entry, "path": child_rel, "type": "file"})
        return node

    if not os.path.isdir(plan_dir):
        return {"name": "plan", "path": "_internal_documents/plan", "type": "dir", "children": []}
    return walk(plan_dir, "_internal_documents/plan")


@app.get("/plans")
def get_plans():
    """Return the plan/ directory tree as JSON."""
    return JSONResponse(_scan_plans())


@app.get("/plans/content")
def get_plan_content(path: str = ""):
    """Return the text content of a .md file inside plan/.
    'path' must be a repo-relative forward-slash path."""
    if not path:
        return JSONResponse({"error": "path is required."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    full = os.path.realpath(os.path.join(repo_root, path.replace('/', os.sep)))
    allowed = os.path.realpath(os.path.join(repo_root, "_internal_documents", "plan"))
    if not full.startswith(allowed + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(full):
        return JSONResponse({"error": "File not found."}, status_code=404)
    try:
        with open(full, encoding='utf-8', errors='replace') as fh:
            content = fh.read()
        return JSONResponse({"path": path, "name": os.path.basename(full), "content": content})
    except Exception as e:
        return JSONResponse({"error": str(e)}, status_code=500)


@app.get("/client-portal/{folder}")
def serve_client_portal(folder: str):
    """Serve _interfaces/business_suite/client_fe/{folder}/index.html."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "_interfaces", "business_suite", "client_fe"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this portal."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


@app.get("/middle-wear")
def list_middle_wear():
    """List all middleware tools inside _interfaces/business_suite/middle_wear/."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    mw_dir = os.path.join(repo_root, "_interfaces", "business_suite", "middle_wear")
    if not os.path.isdir(mw_dir):
        return JSONResponse({"tools": []})
    tools = []
    for entry in sorted(os.listdir(mw_dir)):
        tool_path = os.path.join(mw_dir, entry)
        if not os.path.isdir(tool_path):
            continue
        has_index = os.path.isfile(os.path.join(tool_path, "index.html"))
        tools.append({
            "folder":    entry,
            "has_index": has_index,
        })
    return JSONResponse({"tools": tools})


@app.get("/middle-portal/{folder}")
def serve_middle_portal(folder: str):
    """Serve business_suite/middle_wear/{folder}/index.html."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "business_suite", "middle_wear"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this tool."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


@app.get("/demos")
def list_demos():
    """List all demo workspaces inside _internal_workspace/demo_workspaces/."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    demos_dir = os.path.join(repo_root, "_internal_workspace", "demo_workspaces")
    if not os.path.isdir(demos_dir):
        return JSONResponse({"demos": []})
    demos = []
    for entry in sorted(os.listdir(demos_dir)):
        demo_path = os.path.join(demos_dir, entry)
        if not os.path.isdir(demo_path):
            continue
        has_index = os.path.isfile(os.path.join(demo_path, "index.html"))
        meta_path = os.path.join(demo_path, "demo.json")
        title = entry.replace("_", " ").title()
        description = None
        icon = "🗂"
        if os.path.isfile(meta_path):
            try:
                import json as _json
                with open(meta_path, "r", encoding="utf-8") as f:
                    meta = _json.load(f)
                title = meta.get("title", title)
                description = meta.get("description", description)
                icon = meta.get("icon", icon)
                libraries = meta.get("libraries", [])
            except Exception:
                pass
        demos.append({
            "folder":      entry,
            "title":       title,
            "description": description,
            "icon":        icon,
            "has_index":   has_index,
            "libraries":   libraries,
        })
    return JSONResponse({"demos": demos})


@app.post("/demos/new")
async def create_demo(request: Request):
    """Create a new blank demo workspace folder.
    Body: { name: str }  — used as the folder name (sanitised)."""
    import uuid as _uuid_mod
    import json as _json
    body = await request.json()
    raw_name = str(body.get("name", "")).strip()
    if not raw_name:
        return JSONResponse({"error": "name is required."}, status_code=400)
    folder = re.sub(r'[^\w]+', '_', raw_name).strip('_').lower()
    if not folder:
        return JSONResponse({"error": "Invalid name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    demos_dir = os.path.join(repo_root, "_internal_workspace", "demo_workspaces")
    target = os.path.realpath(os.path.join(demos_dir, folder))
    if not target.startswith(os.path.realpath(demos_dir) + os.sep):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    if os.path.exists(target):
        return JSONResponse({"error": "A demo with that name already exists."}, status_code=409)
    os.makedirs(target, exist_ok=True)
    with open(os.path.join(target, "demo.json"), "w", encoding="utf-8") as f:
        _json.dump({"title": raw_name, "description": "", "icon": "🗂"}, f, indent=2)
    with open(os.path.join(target, "index.html"), "w", encoding="utf-8") as f:
        f.write(f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <title>{raw_name}</title>
  <style>
    body {{ font-family: system-ui, sans-serif; display: flex; align-items: center;
            justify-content: center; height: 100vh; margin: 0; background: #f8fafc; color: #64748b; }}
    h1 {{ font-size: 1.4rem; font-weight: 700; }}
  </style>
</head>
<body><h1>✏️ {raw_name}</h1></body>
</html>
""")
    return JSONResponse({"success": True, "folder": folder})


@app.get("/demo/{folder}/source")
def serve_demo_source(folder: str):
    """Return the raw source of demo_workspaces/{folder}/index.html as plain text."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "_internal_workspace", "demo_workspaces"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found."}, status_code=404)
    with open(page, "r", encoding="utf-8") as f:
        content = f.read()
    from starlette.responses import PlainTextResponse
    return PlainTextResponse(content, media_type="text/plain; charset=utf-8")


@app.get("/demo/{folder}")
def serve_demo(folder: str):
    """Serve _internal_workspace/demo_workspaces/{folder}/index.html."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    base = os.path.realpath(os.path.join(repo_root, "_internal_workspace", "demo_workspaces"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this demo."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


@app.get("/file-content")
def serve_file_content(path: str = ""):
    """Return a repository source file as plain text for the demo library viewer.
    path must be relative to the repo root and must resolve within an allowed sub-directory."""
    if not path:
        return JSONResponse({"error": "path is required."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ALLOWED = ("_deliverables/", "_interfaces/", "_sub_repos/")
    norm = path.replace("\\", "/").lstrip("/")
    if not any(norm.startswith(p) for p in ALLOWED):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    resolved = os.path.realpath(os.path.join(repo_root, norm))
    if not resolved.startswith(os.path.realpath(repo_root) + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(resolved):
        return JSONResponse({"error": "File not found."}, status_code=404)
    try:
        with open(resolved, "r", encoding="utf-8") as f:
            content = f.read()
    except Exception as exc:
        return JSONResponse({"error": str(exc)}, status_code=500)
    from starlette.responses import PlainTextResponse
    return PlainTextResponse(content, media_type="text/plain; charset=utf-8")


@app.get("/git/log")
def git_log(n: int = 60, branch: str = ""):
    """Return the last `n` git commits as structured JSON.
    Optional `branch` param filters to a specific branch."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    # Validate inputs
    if not (1 <= n <= 500):
        n = 60
    fmt = "%x1f".join(["%H", "%h", "%s", "%an", "%ae", "%ai", "%D"]) + "%x1e"
    cmd = ["git", "log", f"--max-count={n}", f"--format={fmt}", "--decorate=full"]
    if branch and re.match(r'^[\w\.\-/]+$', branch):
        cmd.append(branch)
    try:
        out = subprocess.check_output(cmd, cwd=repo_root, text=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return JSONResponse({"success": False, "commits": [], "error": "git log failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "commits": [], "error": "git not found"})

    commits = []
    for record in out.strip().split("\x1e"):
        record = record.strip()
        if not record:
            continue
        parts = record.split("\x1f")
        if len(parts) < 6:
            continue
        sha, short, subject, author, email, date_iso = parts[:6]
        refs = parts[6] if len(parts) > 6 else ""
        # Parse branch/tag refs
        tags = [r.strip().removeprefix("refs/tags/") for r in refs.split(",")
                if "refs/tags/" in r]
        branches = [r.strip().removeprefix("refs/heads/")
                    .removeprefix("refs/remotes/")
                    for r in refs.split(",")
                    if "refs/heads/" in r or "refs/remotes/" in r]
        commits.append({
            "sha": sha.strip(),
            "short": short.strip(),
            "subject": subject.strip(),
            "author": author.strip(),
            "email": email.strip(),
            "date": date_iso.strip(),
            "branches": branches,
            "tags": tags,
        })

    # Also return list of local branches for the branch switcher
    try:
        branches_raw = subprocess.check_output(
            ["git", "branch", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
    except Exception:
        branches_raw = []

    return JSONResponse({"success": True, "commits": commits, "branches": branches_raw})


@app.get("/git/diff")
def git_diff(ref: str = ""):
    """Return a unified diff.
    If `ref` is a valid SHA, shows that commit via `git show`.
    Otherwise shows working tree vs HEAD via `git diff HEAD`."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if ref and re.match(r'^[0-9a-fA-F]{4,40}$', ref):
        cmd = ["git", "show", ref, "--no-color", "--patch"]
        label = ref
    else:
        cmd = ["git", "diff", "HEAD", "--no-color"]
        label = "working tree vs HEAD"
    try:
        diff = subprocess.check_output(
            cmd, cwd=repo_root, stderr=subprocess.DEVNULL,
            encoding='utf-8', errors='replace',
        )
    except subprocess.CalledProcessError as e:
        diff = e.output or ""
    except FileNotFoundError:
        return JSONResponse({"success": False, "diff": "", "ref": label, "error": "git not found"})
    except Exception as e:
        return JSONResponse({"success": False, "diff": "", "ref": label, "error": str(e)})
    return JSONResponse({"success": True, "diff": diff, "ref": label})


@app.get("/git/stash")
def git_stash_list():
    """Return the git stash list as structured entries."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        raw = subprocess.check_output(
            ["git", "stash", "list", "--format=%gd\x1f%s\x1f%ci"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return JSONResponse({"success": False, "entries": [], "error": "git stash list failed"})

    entries = []
    for line in raw.splitlines():
        parts = line.split("\x1f")
        ref = parts[0].strip() if len(parts) > 0 else ""
        msg = parts[1].strip() if len(parts) > 1 else ""
        date = parts[2].strip() if len(parts) > 2 else ""
        entries.append({"ref": ref, "message": msg, "date": date})
    return JSONResponse({"success": True, "entries": entries})


@app.get("/git/stash/{index}")
def git_stash_show(index: int):
    """Return the unified diff for stash@{index}."""
    if not (0 <= index <= 99):
        return JSONResponse({"success": False, "diff": "", "error": "Invalid stash index"}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        diff = subprocess.check_output(
            ["git", "stash", "show", "-p", "--no-color", f"stash@{{{index}}}"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        diff = ""
    except FileNotFoundError:
        return JSONResponse({"success": False, "diff": "", "error": "git not found"})
    return JSONResponse({"success": True, "diff": diff})


@app.get("/git/remotes")
def git_remotes():
    """Return all configured git remotes with their fetch and push URLs."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        raw = subprocess.check_output(
            ["git", "remote", "-v"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return JSONResponse({"success": False, "remotes": [], "error": "git remote failed"})

    seen: dict = {}
    for line in raw.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        name, url, kind = parts[0], parts[1], parts[2].strip("()")
        if name not in seen:
            seen[name] = {"name": name, "fetch": "", "push": ""}
        seen[name][kind] = url

    return JSONResponse({"success": True, "remotes": list(seen.values())})


@app.get("/git/branches")
def git_branches_all():
    """Return local and remote branches plus the current branch."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        local_raw = subprocess.check_output(
            ["git", "branch", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
        remote_raw = subprocess.check_output(
            ["git", "branch", "-r", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
        current = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        return JSONResponse({"success": False, "local": [], "remote": [], "current": "", "error": str(exc)})

    return JSONResponse({
        "success": True,
        "local": [b.strip() for b in local_raw if b.strip()],
        "remote": [b.strip() for b in remote_raw if b.strip()],
        "current": current,
    })


@app.post("/git/checkout")
async def git_checkout(request: Request):
    """Check out a local branch by name."""
    body = await request.json()
    branch = body.get("branch", "").strip()
    if not branch or not re.match(r'^[\w\.\-/]+$', branch):
        return JSONResponse({"success": False, "error": "Invalid branch name"}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        out = subprocess.check_output(
            ["git", "checkout", branch],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out, "branch": branch})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "checkout failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@app.post("/git/commit")
async def git_commit(request: Request):
    """Stage all changes (git add -A) and commit with the given message."""
    body = await request.json()
    message = body.get("message", "").strip()
    if not message:
        return JSONResponse({"success": False, "error": "Commit message is required"}, status_code=400)
    # Sanitise: no shell injection possible since we pass args as a list, but
    # still reject unusually short/empty messages caught above.
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    try:
        subprocess.check_output(
            ["git", "add", "-A"],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        out = subprocess.check_output(
            ["git", "commit", "-m", message],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "commit failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@app.post("/git/merge")
async def git_merge(request: Request):
    """Merge a branch into the current branch (git merge --no-ff <branch>)."""
    body = await request.json()
    source = body.get("branch", "").strip()
    strategy = body.get("strategy", "no-ff").strip()
    if not source or not re.match(r'^[\w\.\-/]+$', source):
        return JSONResponse({"success": False, "error": "Invalid branch name"}, status_code=400)
    if strategy not in ("no-ff", "ff", "squash"):
        return JSONResponse({"success": False, "error": "Invalid strategy"}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    flag = {"no-ff": "--no-ff", "ff": "--ff-only", "squash": "--squash"}[strategy]
    try:
        current = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
        out = subprocess.check_output(
            ["git", "merge", flag, source],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out, "source": source, "target": current})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "merge failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@app.get("/git/sub-repos")
async def git_sub_repos():
    """Return status of each repo inside _sub_repos/."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sub_repos_dir = os.path.join(repo_root, "_sub_repos")
    results = []
    if not os.path.isdir(sub_repos_dir):
        return JSONResponse({"success": True, "repos": []})
    for name in sorted(os.listdir(sub_repos_dir)):
        path = os.path.join(sub_repos_dir, name)
        if not os.path.exists(os.path.join(path, ".git")):
            continue
        entry = {"name": name, "path": os.path.join("_sub_repos", name)}
        try:
            entry["branch"] = subprocess.check_output(
                ["git", "rev-parse", "--abbrev-ref", "HEAD"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            ).strip()
            log = subprocess.check_output(
                ["git", "log", "-1", "--format=%H%x00%s%x00%an%x00%ai"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            ).strip()
            if log:
                sha, subject, author, date = log.split("\x00", 3)
                entry["sha"] = sha[:8]
                entry["subject"] = subject
                entry["author"] = author
                entry["date"] = date.strip()
            else:
                entry["sha"] = entry["subject"] = entry["author"] = entry["date"] = ""
            status_out = subprocess.check_output(
                ["git", "status", "--porcelain"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            )
            lines_out = [l for l in status_out.splitlines() if l.strip()]
            entry["dirty"] = len(lines_out) > 0
            entry["changed_files"] = len(lines_out)
            remotes_out = subprocess.check_output(
                ["git", "remote", "-v"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            )
            remote_urls = {}
            for rline in remotes_out.splitlines():
                parts = rline.split()
                if len(parts) >= 2 and parts[0] not in remote_urls:
                    remote_urls[parts[0]] = parts[1]
            entry["remotes"] = remote_urls
        except (subprocess.CalledProcessError, FileNotFoundError, ValueError):
            entry.setdefault("branch", "unknown")
            entry.setdefault("sha", "")
            entry.setdefault("subject", "")
            entry.setdefault("author", "")
            entry.setdefault("date", "")
            entry.setdefault("dirty", False)
            entry.setdefault("changed_files", 0)
            entry.setdefault("remotes", {})
        results.append(entry)
    return JSONResponse({"success": True, "repos": results})


@app.post("/experiment/new-workspace")
async def experiment_new_workspace():
    """Create a fresh UUID-named folder inside _internal_workspace/temporary_workspaces/.
    Returns { success, path, uuid } where `path` is repo-relative."""
    import uuid as _uuid_mod
    repo_root = _ws_repo_root()
    base_dir = os.path.join(repo_root, "_internal_workspace", "temporary_workspaces")
    os.makedirs(base_dir, exist_ok=True)
    folder_uuid = str(_uuid_mod.uuid4())
    os.makedirs(os.path.join(base_dir, folder_uuid), exist_ok=True)
    rel = "_internal_workspace/temporary_workspaces/" + folder_uuid
    return JSONResponse({"success": True, "path": rel, "uuid": folder_uuid})

# â”€â”€ Network / local-service discovery â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
import threading as _threading

_service_pids: dict = {}
_service_pids_lock = _threading.Lock()

# Known locally-managed services, grouped for the Services tab in Package Builder.
_KNOWN_LOCAL_SERVICES = [
    # ── Distribution Storage ────────────────────────────────────────────
    {
        "id":           "distribution_storage_node",
        "group":        "Distribution Storage",
        "label":        "Storage · Node",
        "exe":          "distribution_storage_node",
        "description":  "Block-store node — stores and serves file chunks, participates "
                        "in replication, and registers with the metadata server.",
        "default_args": [],
    },
    {
        "id":           "distribution_storage_meta",
        "group":        "Distribution Storage",
        "label":        "Storage · Metadata",
        "exe":          "distribution_storage_meta",
        "description":  "Metadata server — tracks chunk locations, manages namespace, "
                        "coordinates replication across storage nodes.",
        "default_args": [],
    },
    {
        "id":           "distribution_storage_shell",
        "group":        "Distribution Storage",
        "label":        "Storage · DFS Shell",
        "exe":          "distribution_storage_shell",
        "description":  "Interactive DFS shell — browse, read, write and manage "
                        "files in the distributed storage cluster.",
        "default_args": [],
    },
    # ── Distribution Tag ────────────────────────────────────────────
    {
        "id":           "distribution_tag_master",
        "group":        "Distribution Tag",
        "label":        "Distribution Tag · Master",
        "exe":          "distribution_tag_master",
        "description":  "Orchestrator — enqueues tagging jobs, monitors worker "
                        "health via ServiceRegistry, applies CircuitBreaker fault isolation.",
        "default_args": [],
    },
    {
        "id":           "distribution_tag_worker",
        "group":        "Distribution Tag",
        "label":        "Distribution Tag · Worker",
        "exe":          "distribution_tag_worker",
        "description":  "Worker — registers with ServiceRegistry, consumes jobs "
                        "from TaskQueue, acquires DistributedLock per result write.",
        "default_args": ["worker-standalone"],
    },
    # ── Language Servers (LSP) ────────────────────────────────────────
    {
        "id":           "plang_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · PLang",
        "exe":          "plang_lsp",
        "description":  "Language server for PLang — hover, completion, diagnostics "
                        "and go-to-definition via the LSP protocol.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plrust_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · Rust",
        "exe":          "plrust_lsp",
        "description":  "Language server for Rust — LSP front-end wrapping the "
                        "plrust_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "pljava_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · Java",
        "exe":          "pljava_lsp",
        "description":  "Language server for Java — LSP front-end wrapping the "
                        "pljava_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plpython_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · Python",
        "exe":          "plpython_lsp",
        "description":  "Language server for Python — LSP front-end wrapping the "
                        "plpython_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plvlang_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · Vlang",
        "exe":          "plvlang_lsp",
        "description":  "Language server for Vlang — LSP front-end wrapping the "
                        "plvlang_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plc3_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · C3",
        "exe":          "plc3_lsp",
        "description":  "Language server for C3 — LSP front-end wrapping the "
                        "plc3_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plvhdl_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · VHDL",
        "exe":          "plvhdl_lsp",
        "description":  "Language server for VHDL — LSP front-end wrapping the "
                        "plvhdl_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
    {
        "id":           "plmatlab_lsp",
        "group":        "Language Servers (LSP)",
        "label":        "LSP · MATLAB",
        "exe":          "plmatlab_lsp",
        "description":  "Language server for MATLAB — LSP front-end wrapping the "
                        "plmatlab_lsp_lib analysis backend.",
        "default_args": ["--stdio"],
    },
]


def _pid_alive(pid: int) -> bool:
    """Return True if the process with the given PID is still running."""
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


@app.get("/network/local-services")
def network_local_services():
    """Return known local services with their current running status."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    build_dir = os.path.join(repo_root, "build")
    result = []
    with _service_pids_lock:
        for svc in _KNOWN_LOCAL_SERVICES:
            exe_path = _find_product_exe(svc["exe"], build_dir)
            pid = _service_pids.get(svc["id"])
            running = bool(pid and _pid_alive(pid))
            if not running and pid:
                _service_pids.pop(svc["id"], None)
                pid = None
            result.append({
                "id":          svc["id"],
                "group":       svc.get("group", ""),
                "label":       svc["label"],
                "exe":         svc["exe"],
                "description": svc["description"],
                "built":       bool(exe_path),
                "running":     running,
                "pid":         pid,
            })
    return JSONResponse({"success": True, "services": result})


@app.post("/network/local-services/launch")
async def network_launch_service(request: Request):
    """Launch a known local service as a detached process. Body: {id}"""
    body = await request.json()
    svc_id = body.get("id", "").strip()
    svc = next((s for s in _KNOWN_LOCAL_SERVICES if s["id"] == svc_id), None)
    if svc is None:
        return JSONResponse({"success": False, "error": "Unknown service id."}, status_code=400)
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    build_dir = os.path.join(repo_root, "build")
    exe_path = _find_product_exe(svc["exe"], build_dir)
    if not exe_path:
        return JSONResponse({"success": False,
                             "error": f"'{svc['exe']}' not found in build/. Build it first."})
    try:
        cmd = [exe_path] + svc.get("default_args", [])
        if platform.system() == "Windows":
            DETACHED_PROCESS = 0x00000008
            CREATE_NEW_PROCESS_GROUP = 0x00000200
            proc = subprocess.Popen(
                cmd,
                cwd=os.path.dirname(exe_path),
                creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                close_fds=True,
            )
        else:
            proc = subprocess.Popen(
                cmd,
                cwd=os.path.dirname(exe_path),
                start_new_session=True,
                close_fds=True,
            )
        with _service_pids_lock:
            _service_pids[svc["id"]] = proc.pid
        return JSONResponse({"success": True, "pid": proc.pid, "label": svc["label"]})
    except Exception as e:
        return JSONResponse({"success": False, "error": str(e)})


@app.post("/network/local-services/stop")
async def network_stop_service(request: Request):
    """Terminate a tracked local service. Body: {id}"""
    body = await request.json()
    svc_id = body.get("id", "").strip()
    with _service_pids_lock:
        pid = _service_pids.get(svc_id)
    if not pid:
        return JSONResponse({"success": False,
                             "error": "Service not tracked or already stopped."})
    try:
        if platform.system() == "Windows":
            import ctypes
            handle = ctypes.windll.kernel32.OpenProcess(1, False, pid)  # type: ignore
            ctypes.windll.kernel32.TerminateProcess(handle, 1)           # type: ignore
            ctypes.windll.kernel32.CloseHandle(handle)                   # type: ignore
        else:
            import signal
            os.kill(pid, signal.SIGTERM)
        with _service_pids_lock:
            _service_pids.pop(svc_id, None)
        return JSONResponse({"success": True, "pid": pid})
    except Exception as e:
        return JSONResponse({"success": False, "error": str(e)})


@app.get("/network/network-services")
def network_services():
    """Return simulated ServiceRegistry entries â€” what distribution_tag registers at runtime.
    Health reflects whether the matching process is tracked as running in this session."""
    with _service_pids_lock:
        master_alive = bool(
            _service_pids.get("distribution_tag_master") and
            _pid_alive(_service_pids["distribution_tag_master"])
        )
        worker_alive = bool(
            _service_pids.get("distribution_tag_worker") and
            _pid_alive(_service_pids["distribution_tag_worker"])
        )

    services = []
    if master_alive:
        services.append({
            "service_name": "master",
            "instance_id":  "master-0",
            "host":         "localhost",
            "port":         9000,
            "metadata":     "role=master",
            "healthy":      True,
        })
    if worker_alive:
        services.append({
            "service_name": "workers",
            "instance_id":  "worker-standalone",
            "host":         "localhost",
            "port":         0,
            "metadata":     "role=worker",
            "healthy":      True,
        })

    return JSONResponse({"success": True, "services": services})


@app.get("/network/ports")
def list_active_ports():
    """List all actively bound local ports (OS-independent via psutil)."""
    import socket as _socket
    try:
        import psutil
    except ImportError:
        return JSONResponse({
            "ports":  [],
            "source": "unavailable",
            "error":  "psutil not installed â€” run: pip install psutil",
        })
    try:
        conns = psutil.net_connections(kind='inet')
    except psutil.AccessDenied:
        try:
            conns = psutil.Process().connections(kind='inet')
        except Exception as exc:
            return JSONResponse({"ports": [], "source": "error", "error": str(exc)})
    except Exception as exc:
        return JSONResponse({"ports": [], "source": "error", "error": str(exc)})

    seen: dict = {}
    for c in conns:
        if not c.laddr:
            continue
        port = c.laddr.port
        # Prefer LISTEN entries when the same port appears multiple times
        if port not in seen or c.status == "LISTEN":
            seen[port] = c

    result = []
    for port, c in seen.items():
        pid = c.pid
        proc_name = None
        if pid:
            try:
                proc_name = psutil.Process(pid).name()
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass
        proto  = "TCP" if c.type == _socket.SOCK_STREAM else "UDP"
        status = c.status if c.status else ""
        result.append({
            "port":    port,
            "host":    c.laddr.ip or "0.0.0.0",
            "pid":     pid,
            "process": proc_name,
            "status":  status,
            "proto":   proto,
        })
    result.sort(key=lambda x: x["port"])
    return JSONResponse({"ports": result, "source": "psutil"})


# â”€â”€ Workspace file editor endpoints â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

def _ws_repo_root() -> str:
    return os.path.realpath(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

def _ws_resolve(rel_path: str):
    """Resolve a workspace-relative path safely. Returns (abs_path, error_response)."""
    root = _ws_repo_root()
    # Normalise: drop leading slashes / backslashes
    clean = rel_path.lstrip("/\\")
    target = os.path.realpath(os.path.join(root, clean))
    if target != root and not target.startswith(root + os.sep):
        return None, JSONResponse({"error": "Access denied."}, status_code=403)
    return target, None

_WS_IGNORE = {
    ".git", ".venv", "venv", "__pycache__", "node_modules",
    "build", ".mypy_cache", ".pytest_cache",
}
_WS_MAX_READ = 2 * 1024 * 1024   # 2 MB cap for reads
_WS_MAX_WRITE = 2 * 1024 * 1024  # 2 MB cap for writes

_WS_TEXT_EXTS = {
    "c","cc","cpp","cxx","h","hpp","hxx","inl",
    "py","pyi","pyx","pxd",
    "js","mjs","ts","tsx","jsx","cjs",
    "json","jsonc","toml","yaml","yml","xml","xsd","dtd",
    "md","rst","txt","csv","log",
    "html","htm","css","scss","less","svg",
    "sh","bash","zsh","fish","ps1","bat","cmd",
    "cmake","make","mk","dockerfile","makefile",
    "go","rs","java","kt","swift","rb","php","lua","zig","v",
    "sql","graphql","proto","tf","tfvars",
}

@app.get("/workspace/tree")
def workspace_tree(path: str = ""):
    """List a directory inside the repo. Returns files + subdirs."""
    target, err = _ws_resolve(path)
    if err:
        return err
    if not os.path.isdir(target):
        return JSONResponse({"error": "Not a directory."}, status_code=400)

    root = _ws_repo_root()
    entries = []
    try:
        names = sorted(os.listdir(target), key=lambda n: (not os.path.isdir(os.path.join(target, n)), n.lower()))
    except PermissionError:
        return JSONResponse({"error": "Permission denied."}, status_code=403)

    for name in names:
        if name.startswith('.') and name not in ('.gitignore', '.env.example'):
            continue
        if name in _WS_IGNORE:
            continue
        full = os.path.join(target, name)
        rel  = os.path.relpath(full, root).replace("\\", "/")
        ext  = name.rsplit(".", 1)[-1].lower() if "." in name else ""
        is_dir = os.path.isdir(full)
        entry = {
            "name": name,
            "path": rel,
            "type": "dir" if is_dir else "file",
        }
        if not is_dir:
            try:
                entry["size"] = os.path.getsize(full)
            except OSError:
                entry["size"] = 0
            entry["editable"] = ext in _WS_TEXT_EXTS or ext == ""
        entries.append(entry)

    rel_self = os.path.relpath(target, root).replace("\\", "/")
    return JSONResponse({"path": rel_self, "entries": entries})


@app.get("/workspace/read")
def workspace_read(path: str):
    """Read a text file from the repo. Returns its content as UTF-8."""
    if not path:
        return JSONResponse({"error": "path required."}, status_code=400)
    target, err = _ws_resolve(path)
    if err:
        return err
    if not os.path.isfile(target):
        return JSONResponse({"error": "File not found."}, status_code=404)

    size = os.path.getsize(target)
    if size > _WS_MAX_READ:
        return JSONResponse({"error": f"File too large ({size} bytes, max {_WS_MAX_READ})."}, status_code=413)

    try:
        with open(target, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
    except PermissionError:
        return JSONResponse({"error": "Permission denied."}, status_code=403)

    root = _ws_repo_root()
    return JSONResponse({
        "path":    os.path.relpath(target, root).replace("\\", "/"),
        "content": content,
        "size":    size,
    })


@app.post("/workspace/write")
async def workspace_write(request: Request):
    """Write content to a file in the repo (UTF-8). Body: {path, content}."""
    try:
        body = await request.json()
    except Exception:
        return JSONResponse({"error": "Invalid JSON body."}, status_code=400)

    rel_path = body.get("path", "")
    content  = body.get("content", "")
    if not rel_path:
        return JSONResponse({"error": "path required."}, status_code=400)
    if not isinstance(content, str):
        return JSONResponse({"error": "content must be a string."}, status_code=400)
    if len(content.encode("utf-8")) > _WS_MAX_WRITE:
        return JSONResponse({"error": "Content too large."}, status_code=413)

    target, err = _ws_resolve(rel_path)
    if err:
        return err
    if not os.path.isfile(target):
        return JSONResponse({"error": "File not found (create not supported)."}, status_code=404)

    ext = target.rsplit(".", 1)[-1].lower() if "." in os.path.basename(target) else ""
    if ext not in _WS_TEXT_EXTS and ext != "":
        return JSONResponse({"error": "File type not editable."}, status_code=415)

    try:
        with open(target, "w", encoding="utf-8", newline="") as f:
            f.write(content)
    except PermissionError:
        return JSONResponse({"error": "Permission denied."}, status_code=403)

    root = _ws_repo_root()
    return JSONResponse({
        "success": True,
        "path":    os.path.relpath(target, root).replace("\\", "/"),
        "bytes":   len(content.encode("utf-8")),
    })


def _docs_html_dir() -> str:
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    return os.path.join(repo_root, "gen_docs", "html")


@app.get("/docs/status")
def docs_status():
    """Check whether Doxygen output exists and return basic stats."""
    html_dir = _docs_html_dir()
    index = os.path.join(html_dir, "index.html")
    if not os.path.isfile(index):
        return JSONResponse({"exists": False, "file_count": 0, "last_built": None})
    # Count HTML pages (rough quality indicator)
    html_files = [f for f in os.listdir(html_dir) if f.endswith(".html")]
    mtime = os.path.getmtime(index)
    import datetime
    last_built = datetime.datetime.fromtimestamp(mtime).isoformat(timespec="seconds")
    return JSONResponse({"exists": True, "file_count": len(html_files), "last_built": last_built})


@app.post("/docs/build")
async def docs_build():
    """Run Doxygen using the repo-root Doxyfile.
    If doxygen is not on PATH, automatically runs the OS-appropriate install
    script via _scripts/script_library_runner.py, then retries.
    Returns {success, output, returncode}."""
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    doxyfile = os.path.join(repo_root, "Doxyfile")
    if not os.path.isfile(doxyfile):
        return JSONResponse({"success": False, "output": "Doxyfile not found in repo root."})

    def _run_doxygen() -> subprocess.CompletedProcess:
        return subprocess.run(
            ["doxygen", "Doxyfile"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            timeout=300,
        )

    install_output = ""
    try:
        result = _run_doxygen()
    except FileNotFoundError:
        # doxygen not on PATH â€” try the OS-appropriate install script
        runner = os.path.join(repo_root, "_scripts", "script_library_runner.py")
        install_cmd = [sys.executable, runner, "--tool", "doxygen"]
        install_output = f"doxygen not found â€” running: {' '.join(install_cmd)}\n\n"
        try:
            ir = subprocess.run(
                install_cmd,
                cwd=repo_root,
                capture_output=True,
                text=True,
                timeout=180,
            )
            install_output += ir.stdout + ir.stderr
            if ir.returncode != 0:
                install_output += (
                    "\n\nAuto-install failed. To install manually:\n"
                    f"  python _scripts/script_library_runner.py --tool doxygen\n"
                    f"  python _scripts/script_library_runner.py --list-tools"
                )
                return JSONResponse({"success": False, "output": install_output, "returncode": ir.returncode})
        except Exception as e:
            return JSONResponse({"success": False,
                                 "output": install_output + f"\nRunner error: {e}",
                                 "returncode": -1})
        # Retry doxygen after install
        try:
            result = _run_doxygen()
            install_output += "\n\n"
        except FileNotFoundError:
            return JSONResponse({
                "success": False,
                "output": install_output + (
                    "\ndoxygen still not found after install. "
                    "Restart the server so the new PATH is picked up."
                ),
                "returncode": -1,
            })
    except subprocess.TimeoutExpired:
        return JSONResponse({"success": False, "output": "Doxygen timed out after 300s.", "returncode": -1})

    output = install_output + result.stdout + result.stderr
    return JSONResponse({"success": result.returncode == 0, "output": output, "returncode": result.returncode})


@app.get("/docs-preview/{file_path:path}")
def docs_preview(file_path: str):
    """Serve files from gen_docs/html/ safely (no path traversal)."""
    html_dir = _docs_html_dir()
    # Default to index.html for bare /docs-preview/
    if not file_path or file_path == "/":
        file_path = "index.html"
    # Resolve and guard against traversal
    target = os.path.realpath(os.path.join(html_dir, file_path))
    if not target.startswith(os.path.realpath(html_dir)):
        return JSONResponse({"error": "Forbidden"}, status_code=403)
    if not os.path.isfile(target):
        return JSONResponse({"error": "Not found"}, status_code=404)
    return StarletteFileResponse(target)

