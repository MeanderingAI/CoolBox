from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
import os
import re
from ._utils import _repo_root
import makefile_manager as mm

router = APIRouter()

@router.get("/deps/{target}")
def get_deps(target: str):
    if not re.match(r'^[\w\-\.]+$', target):
        return JSONResponse({"success": False, "edges": [], "dot": None,
                             "output": "Invalid target.", "available_targets": []}, status_code=400)
    return JSONResponse(mm.get_deps(target))
