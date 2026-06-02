import os
import sys
import subprocess
from fastapi import APIRouter, Request
from fastapi.responses import PlainTextResponse, JSONResponse
from __init__ import REPO_ROOT, DSN_PATH, SCRIPT_PATH

router = APIRouter()


@router.get("/api/pipeline/dsn")
def get_postgres_dsn():
    if os.path.isfile(DSN_PATH):
        with open(DSN_PATH, "r", encoding="utf-8") as f:
            return JSONResponse({"dsn": f.read().strip()})
    return JSONResponse({"dsn": ""})

@router.post("/api/pipeline/dsn")
async def set_postgres_dsn(request: Request):
    data = await request.json()
    dsn = data.get("dsn", "").strip()
    os.makedirs(os.path.dirname(DSN_PATH), exist_ok=True)
    with open(DSN_PATH, "w", encoding="utf-8") as f:
        f.write(dsn)
    return JSONResponse({"success": True, "dsn": dsn})

@router.post("/api/pipeline/preview")
def run_pipeline_preview():
    """Run the local pipeline preview script and return its output."""
    # Load DSN if present
    if os.path.isfile(DSN_PATH):
        dsn = open(DSN_PATH, encoding="utf-8").read().strip()
        if dsn:
            os.environ["POSTGRES_DSN"] = dsn
    if not os.path.isfile(SCRIPT_PATH):
        return PlainTextResponse("Script not found: " + SCRIPT_PATH, status_code=404)
    try:
        result = subprocess.run([sys.executable, SCRIPT_PATH], capture_output=True, text=True, check=True)
        return PlainTextResponse(result.stdout)
    except subprocess.CalledProcessError as e:
        return PlainTextResponse("Pipeline preview failed:\n" + e.stderr, status_code=500)

@router.get("/api/pipeline/yaml")
def get_pipeline_yaml():
    """Return the generated YAML output (if any)."""
    yaml_path = os.path.join(REPO_ROOT, "_local_build_pipeline", "tmp", "preview_github_workflow.yaml")
    if not os.path.isfile(yaml_path):
        return PlainTextResponse("No YAML output found. Run preview first.", status_code=404)
    with open(yaml_path, "r", encoding="utf-8") as f:
        return PlainTextResponse(f.read())
