
from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse as StarletteFileResponse
import os
import re
import makefile_manager as mm
from __init__ import REPO_ROOT

app = APIRouter()


@app.get("/demo-assets/fourier_fft_demo/module/{asset_name}")
def serve_fourier_demo_module_asset(asset_name: str):
    """Serve built Emscripten Fourier assets for the Fourier FFT demo."""
    if not re.match(r'^[\w\-.]+$', asset_name):
        return JSONResponse({"error": "Invalid asset name."}, status_code=400)
    if asset_name not in {"fourier_tranforms.js", "fourier_tranforms.wasm"}:
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
        return JSONResponse({"error": f"{asset_name} not found. Build target fourier_tranforms_js first."}, status_code=404)

    return StarletteFileResponse(asset)


@app.post("/demo-assets/fourier_fft_demo/module/build")
def build_fourier_demo_module_assets():
    """Build Fourier transform Emscripten assets for the Fourier FFT demo."""
    result = mm.build_target("fourier_tranforms_js", timeout=1200)

    repo_root = REPO_ROOT
    js_path = os.path.realpath(os.path.join(
        repo_root,
        "build",
        "_deliverables",
        "libraries",
        "bindings",
        "emscripten_bindings",
        "fourier_tranforms.js",
    ))
    wasm_path = os.path.realpath(os.path.join(
        repo_root,
        "build",
        "_deliverables",
        "libraries",
        "bindings",
        "emscripten_bindings",
        "fourier_tranforms.wasm",
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


@app.get("/demo-assets/{folder}/{asset_path:path}")
def serve_demo_asset(folder: str, asset_path: str):
    """Serve assets stored inside _internal_workspace/demo_workspaces/{folder}/."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    if not asset_path:
        return JSONResponse({"error": "asset_path is required."}, status_code=400)

    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(repo_root, "_internal_workspace", "demo_workspaces", folder))
    asset = os.path.realpath(os.path.join(base, asset_path))

    if not asset.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(asset):
        return JSONResponse({"error": "Asset not found."}, status_code=404)

    return StarletteFileResponse(asset)


@app.get("/demos")
def list_demos():
    """List all demo workspaces inside _internal_workspace/demo_workspaces/."""
    repo_root = REPO_ROOT
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
        libraries = []
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
    repo_root = REPO_ROOT
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
    repo_root = REPO_ROOT
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
    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(repo_root, "_internal_workspace", "demo_workspaces"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this demo."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")
