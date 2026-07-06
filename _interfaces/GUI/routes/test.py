from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse
import os
import re
from ._utils import _repo_root
import makefile_manager as mm

router = APIRouter()

@router.post("/test/build")
async def test_build(request: Request):
    body = await request.json()
    test_name = body.get("name", "").strip()
    build_target = body.get("build_target", "").strip()
    if not re.match(r'^[\w:\.\- ]+$', test_name):
        return JSONResponse({"success": False, "output": "Invalid test name."}, status_code=400)
    cmake_target = build_target if build_target and re.match(r'^[\w\-]+$', build_target) else test_name
    result = mm.build_target(cmake_target, timeout=300)
    return JSONResponse(result)

@router.post("/test/run")
async def test_run(request: Request):
    body = await request.json()
    test_name = body.get("name", "").strip()
    if not re.match(r'^[\w:\.\-]+$', test_name):
        return JSONResponse({"success": False, "output": "Invalid test name."}, status_code=400)
    result = mm.ctest_run(test_name, timeout=180)
    return JSONResponse(result)
