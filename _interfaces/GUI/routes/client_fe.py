from fastapi import APIRouter
from fastapi.responses import JSONResponse
import os
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT

p1 = APIRouter()

@p1.get("/client-fe")
def list_client_fe():
    """List all client portals inside _interfaces/business_suite/client_fe/."""
    repo_root = REPO_ROOT
    client_fe_dir = os.path.join(repo_root, "_interfaces", "business_suite", "client_fe")
    if not os.path.isdir(client_fe_dir):
        return JSONResponse({"portals": []})
    portals: list[dict[str, object]] = []
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