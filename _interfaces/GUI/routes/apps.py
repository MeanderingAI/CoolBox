from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse as StarletteFileResponse
import os
import re
import sys
import platform
import subprocess
import importlib
from typing import Any, cast
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT

mm = cast(Any, importlib.import_module("makefile_manager"))

p1 = APIRouter()

def _find_product_exe(exe_name: str, build_dir: str) -> str:
    """Find a built product exe in build/. Returns absolute path or empty string."""
    exe_variants = [exe_name, exe_name + ".exe"]
    for dirpath, dirs, files in os.walk(build_dir):
        dirs[:] = [d for d in dirs if d not in {"CMakeFiles", ".cmake"}]
        for fname in files:
            if fname in exe_variants:
                return os.path.join(dirpath, fname)
    return ""


def _scan_apps() -> list[dict[str, object]]:
    """Scan _deliverables/apps/ subdirectories. Returns list of app descriptors."""
    repo_root = REPO_ROOT
    apps_dir = os.path.join(repo_root, "_deliverables", "apps")
    apps: list[dict[str, object]] = []
    if not os.path.isdir(apps_dir):
        return apps
    for entry in sorted(os.listdir(apps_dir)):
        sub = os.path.join(apps_dir, entry)
        if not os.path.isdir(sub) or entry.startswith(('_', '.')):
            continue
        # Determine type and targets
        cmake = os.path.join(sub, "CMakeLists.txt")
        executables: list[str] = []
        scripts: list[str] = []
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


@p1.get("/apps")
def get_apps():
    """Return a list of apps from _deliverables/apps/."""
    return JSONResponse({"apps": _scan_apps()})


@p1.post("/apps/build")
async def build_app(request: Request):
    """Build an app cmake target. Body: {name, target}"""
    body = await request.json()
    name = body.get("name", "").strip()
    target = body.get("target", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', target):
        return JSONResponse({"success": False, "output": "Invalid name or target."}, status_code=400)
    result = mm.build_target(target, timeout=300)
    return JSONResponse(result)


@p1.post("/apps/launch")
async def launch_app(request: Request):
    """Launch a built app exe as a detached process. Body: {name, exe}"""
    body = await request.json()
    name = body.get("name", "").strip()
    exe_name = body.get("exe", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', exe_name):
        return JSONResponse({"success": False, "output": "Invalid name or exe."}, status_code=400)
    repo_root = REPO_ROOT
    build_dir = os.path.join(repo_root, "build")
    exe_path = _find_product_exe(exe_name, build_dir)
    if not exe_path:
        return JSONResponse({"success": False,
                             "output": f"'{exe_name}' not found in build/. Build it first."})
    try:
        if platform.system() == "Windows":
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


@p1.post("/apps/run-script")
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
    repo_root = REPO_ROOT
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


@p1.get("/app-page/{folder}")
def app_page(folder: str):
    """Serve the app_page.html for an app in _deliverables/apps/{folder}/."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(repo_root, "_deliverables", "apps"))
    page = os.path.realpath(os.path.join(base, folder, "app_page.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "app_page.html not found for this app."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")
