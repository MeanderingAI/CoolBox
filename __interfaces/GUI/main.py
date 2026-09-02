from fastapi import BackgroundTasks
# --- Sub-Repo Recursive Pull Endpoint ---
from fastapi import BackgroundTasks


import mimetypes
mimetypes.add_type("application/javascript", ".mjs")
mimetypes.add_type("application/javascript", ".js")

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "library"))
import makefile_manager as mm

from fastapi import FastAPI, Request
from fastapi.responses import FileResponse, JSONResponse, StreamingResponse
from routes.build_wasm import router as build_wasm_router
from routes.test import router as test_router
from routes.deps import router as deps_router
from routes.library import router as library_router
from routes.products import p1 as products_router, _find_product_exe
from routes.apps import p1 as apps_router
from routes.client_fe import p1 as client_fe_router
from routes.client_portal import p1 as client_portal_router
from routes.middle import p1 as middle_router
from routes.extensions import p1 as extensions_router
from routes.demo_assets import app as demo_router
from routes.items import router as items_router
from routes.git import p1 as git_router
from routes.dashboard import router as dashboard_router
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




from library import pipeline_preview
from library import pipeline_act


from routes import test, deps, library, products, items, git

app = FastAPI()
app.include_router(pipeline_preview.router)
app.include_router(pipeline_act.router)

app.include_router(build_wasm_router)
app.include_router(test_router)
app.include_router(deps_router)
app.include_router(library_router)
app.include_router(products_router)
app.include_router(apps_router)
app.include_router(client_fe_router)
app.include_router(client_portal_router)
app.include_router(middle_router)
app.include_router(extensions_router)
app.include_router(demo_router)
app.include_router(items_router)
app.include_router(git_router)
app.include_router(dashboard_router)



app.mount("/static", NoCacheStaticFiles(directory="static"), name="static")

# Serve static/index.html at root
from fastapi.responses import FileResponse

@app.get("/")
def root():
    return FileResponse("static/index.html")

# Mount GIF third party static if present (centralized logic)
try:
    from . import mount_gif_third_party
    mount_gif_third_party(app, NoCacheStaticFiles)
except ImportError:
    pass






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

        app = FastAPI()
        app.include_router(pipeline_preview.router)
        app.include_router(pipeline_act.router)
        app.include_router(build_wasm_router)
        app.include_router(test_router)
        app.include_router(deps_router)
        app.include_router(library_router)
        app.include_router(products_router)
        app.include_router(items_router)
        app.include_router(git_router)

        # --- Static mounting ---
        app.mount("/static", NoCacheStaticFiles(directory="static"), name="static")

        # Mount GIF third party static if present (centralized logic)
        try:
            from . import mount_gif_third_party
            mount_gif_third_party(app, NoCacheStaticFiles)
        except ImportError:
            pass
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

