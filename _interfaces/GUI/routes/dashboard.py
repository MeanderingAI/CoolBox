
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../library"))
from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
import getpass
import subprocess
import importlib
from typing import Any, cast
mm = cast(Any, importlib.import_module("makefile_manager"))
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT


router = APIRouter()


def scan_groups() -> list[dict[str, object]]:
    groups_root = os.path.join(REPO_ROOT, "_deliverables", "libraries", "groups")
    if not os.path.isdir(groups_root):
        return []

    groups: list[dict[str, object]] = []
    for group_name in sorted(os.listdir(groups_root)):
        if group_name.startswith((".", "_")):
            continue
        group_dir = os.path.join(groups_root, group_name)
        if not os.path.isdir(group_dir):
            continue

        libs: set[str] = set()
        for root, dirs, files in os.walk(group_dir):
            dirs[:] = [d for d in dirs if not d.startswith((".", "_"))]
            if root == group_dir:
                continue
            if "CMakeLists.txt" in files:
                lib_name = os.path.basename(root)
                if lib_name.lower() not in {"headers", "header", "src", "source", "sources", "include"}:
                    libs.add(lib_name)

        if not libs:
            for entry in sorted(os.listdir(group_dir)):
                if entry.startswith((".", "_")):
                    continue
                child = os.path.join(group_dir, entry)
                if os.path.isdir(child):
                    libs.add(entry)

        groups.append({"name": group_name, "libs": sorted(libs)})

    return groups

@router.get("/dashboard")
def dashboard(request: Request):
    groups = scan_groups()
    try:
        tests: list[dict[str, object]] = mm.scan_tests() if mm else []
    except Exception:
        tests = []
    server_url = str(request.base_url).rstrip("/")
    user = getpass.getuser() if hasattr(getpass, "getuser") else "unknown"
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
