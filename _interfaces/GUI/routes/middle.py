from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse as StarletteFileResponse
import re
import os
import makefile_manager as mm
from __init__ import REPO_ROOT

p1 = APIRouter()




@p1.get("/middle-wear")
def list_middle_wear():
    """List all middleware tools inside _interfaces/business_suite/middle_wear/."""
    repo_root = REPO_ROOT
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


@p1.get("/middle-portal/{folder}")
def serve_middle_portal(folder: str):
    """Serve _interfaces/business_suite/middle_wear/{folder}/index.html."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(repo_root, "_interfaces", "business_suite", "middle_wear"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this tool."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")


@p1.get("/middle-portal-assets/uuid_generation/{asset_name}")
def serve_uuid_generation_asset(asset_name: str):
    """Serve built Emscripten UUID assets (uuid_generation.js/.wasm)."""
    if not re.match(r'^[\w\-.]+$', asset_name):
        return JSONResponse({"error": "Invalid asset name."}, status_code=400)
    if not asset_name.startswith("uuid_generation."):
        return JSONResponse({"error": "Access denied."}, status_code=403)

    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(
        repo_root,
        "build",
        "_deliverables",
        "libraries",
        "bindings",
        "emscripten_bindings",
    ))
    asset = os.path.realpath(os.path.join(base, asset_name))

    if not asset.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(asset):
        return JSONResponse({"error": f"{asset_name} not found. Build target uuid_generation_js first."}, status_code=404)

    return StarletteFileResponse(asset)


@p1.post("/middle-portal-assets/uuid_generation/build")
def build_uuid_generation_assets():
    """Build uuid_generation Emscripten assets and report generated file status."""
    result = mm.build_target("uuid_generation_js", timeout=1200)

    repo_root = REPO_ROOT
    js_path = os.path.realpath(os.path.join(
        repo_root,
        "build",
        "_deliverables",
        "libraries",
        "bindings",
        "emscripten_bindings",
        "uuid_generation.js",
    ))
    wasm_path = os.path.realpath(os.path.join(
        repo_root,
        "build",
        "_deliverables",
        "libraries",
        "bindings",
        "emscripten_bindings",
        "uuid_generation.wasm",
    ))

    js_exists = os.path.isfile(js_path)
    wasm_exists = os.path.isfile(wasm_path)

    return JSONResponse({
        "success": bool(result.get("success")) and js_exists,
        "build_success": bool(result.get("success")),
        "output": result.get("output", ""),
        "returncode": result.get("returncode", -1),
        "js_exists": js_exists,
        "wasm_exists": wasm_exists,
        "js_path": os.path.relpath(js_path, repo_root).replace("\\", "/"),
        "wasm_path": os.path.relpath(wasm_path, repo_root).replace("\\", "/"),
    })

