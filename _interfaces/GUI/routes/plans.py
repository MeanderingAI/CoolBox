from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
import os
import re
from ._utils import _repo_root

p1 = APIRouter()


def _scan_plans() -> dict:
    """Recursively scan plan/ and return a JSON tree of folders and .md files."""
    repo_root = _repo_root()
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


@p1.get("/plans")
def get_plans():
    """Return the plan/ directory tree as JSON."""
    return JSONResponse(_scan_plans())


@p1.get("/plans/content")
def get_plan_content(path: str = ""):
    """Return the text content of a .md file inside plan/.
    'path' must be a repo-relative forward-slash path."""
    if not path:
        return JSONResponse({"error": "path is required."}, status_code=400)
    repo_root = _repo_root()
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