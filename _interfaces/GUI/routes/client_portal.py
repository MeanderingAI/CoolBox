from fastapi import APIRouter
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse as StarletteFileResponse
import re
import os
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT

p1 = APIRouter()

@p1.get("/client-portal/{folder}")
def serve_client_portal(folder: str):
    """Serve _interfaces/business_suite/client_fe/{folder}/index.html."""
    if not re.match(r'^[\w\-]+$', folder):
        return JSONResponse({"error": "Invalid folder name."}, status_code=400)
    repo_root = REPO_ROOT
    base = os.path.realpath(os.path.join(repo_root, "_interfaces", "business_suite", "client_fe"))
    page = os.path.realpath(os.path.join(base, folder, "index.html"))
    if not page.startswith(base + os.sep):
        return JSONResponse({"error": "Access denied."}, status_code=403)
    if not os.path.isfile(page):
        return JSONResponse({"error": "index.html not found for this portal."}, status_code=404)
    return StarletteFileResponse(page, media_type="text/html")