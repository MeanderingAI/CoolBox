from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse as StarletteFileResponse
import os
import re
import platform
import subprocess
import importlib
from typing import Any, cast
mm = cast(Any, importlib.import_module("makefile_manager"))
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT
#from ._utils import _repo_root

p1 = APIRouter()

#@p1.get("/products")
#def get_products():
#    repo_root = _repo_root()
#    product_dir = os.path.join(repo_root, "_deliverables", "Product")
#    products = []
#    if not os.path.isdir(product_dir):
#        return JSONResponse({"products": products})
#    for entry in sorted(os.listdir(product_dir)):
#        sub = os.path.join(product_dir, entry)
#        if not os.path.isdir(sub) or entry.startswith(('_', '.')):
#            continue
#        cmake = os.path.join(sub, "CMakeLists.txt")
#        if not os.path.isfile(cmake):
#            continue
#        try:
#            with open(cmake, encoding='utf-8', errors='replace') as fh:
#                content = fh.read()
#        except Exception:
#            content = ""
#        executables = re.findall(r'add_executable\s*\(\s*(\w+)', content)
#        has_page = os.path.isfile(os.path.join(sub, "product_page.html"))
#        products.append({"name": entry, "folder": entry, "executables": executables,
#                         "has_page": has_page})
#    return JSONResponse({"products": products})

def _scan_products() -> list[dict[str, object]]:
    """Scan _deliverables/Product/ subdirectories and parse cmake exe targets from CMakeLists.txt."""
    repo_root = REPO_ROOT
    product_dir = os.path.join(repo_root, "_deliverables", "Product")
    products: list[dict[str, object]] = []
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
        executables: list[str] = re.findall(r'add_executable\s*\(\s*(\w+)', content)
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


@p1.get("/products")
def get_products():
    """Return a list of buildable/launchable products from _Product/."""
    return JSONResponse({"products": _scan_products()})


@p1.post("/products/build")
async def build_product(request: Request):
    """Build a product exe via cmake --build. Body: {name, target}"""
    body = await request.json()
    name = body.get("name", "").strip()
    target = body.get("target", "").strip()
    if not re.match(r'^[\w\-]+$', name) or not re.match(r'^[\w\-]+$', target):
        return JSONResponse({"success": False, "output": "Invalid name or target."}, status_code=400)
    result = mm.build_target(target, timeout=300)
    return JSONResponse(result)


@p1.post("/products/launch")
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


@p1.get("/product-page/{folder}")
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

