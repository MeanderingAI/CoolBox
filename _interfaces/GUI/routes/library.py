from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
import os
import re
from ._utils import _repo_root

router = APIRouter()

@router.get("/library/info")
def library_info(group: str = "", lib: str = ""):
    if not re.match(r'^[\w\-]+$', group) or not re.match(r'^[\w\-]+$', lib):
        return JSONResponse({"error": "Invalid group or lib name."}, status_code=400)
    repo_root = _repo_root()
    base_dir = os.path.realpath(os.path.join(repo_root, "_deliverables", "libraries", "groups", group, lib))
    allowed = os.path.realpath(os.path.join(repo_root, "_deliverables", "libraries", "groups"))
    if not base_dir.startswith(allowed + os.sep) and base_dir != allowed:
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isdir(base_dir):
        return JSONResponse({"error": f"'{group}/{lib}' not found."}, status_code=404)
    # _scan_lib_dir logic inlined for modularity
    def _parse_cmake_target(cmake_file: str) -> str:
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
        results = []
        headers_dir = os.path.join(dir_path, "headers")
        if os.path.isdir(headers_dir):
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
    libs = _scan_lib_dir(base_dir, repo_root)
    return JSONResponse({"group": group, "lib": lib, "libs": libs})

@router.get("/library/file")
def library_file(path: str = ""):
    repo_root = _repo_root()
    if not path:
        return JSONResponse({"error": "path is required."}, status_code=400)
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
