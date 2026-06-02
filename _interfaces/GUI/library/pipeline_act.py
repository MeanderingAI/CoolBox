import os
import subprocess
import time
from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse, PlainTextResponse
from __init__ import GITHUB_WORKFLOWS, REPO_ROOT

router = APIRouter()


def _docker_ready() -> bool:
    try:
        proc = subprocess.run(
            ["docker", "info"],
            capture_output=True,
            text=True,
            check=False,
        )
        return proc.returncode == 0
    except Exception:
        return False


def _try_start_docker_desktop() -> None:
    if os.name != "nt":
        return
    possible_paths = [
        os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "Docker", "Docker", "Docker Desktop.exe"),
        os.path.join(os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"), "Docker", "Docker", "Docker Desktop.exe"),
    ]
    for exe_path in possible_paths:
        if os.path.isfile(exe_path):
            try:
                os.startfile(exe_path)  # type: ignore[attr-defined]
                return
            except Exception:
                continue


@router.get("/api/pipeline/docker-status")
def docker_status():
    return JSONResponse({"running": _docker_ready()})


@router.post("/api/pipeline/docker-start")
def docker_start():
    _try_start_docker_desktop()
    # Give Docker a short window to initialize.
    for _ in range(20):
        if _docker_ready():
            break
        time.sleep(1)
    return JSONResponse({"running": _docker_ready()})

@router.get("/api/pipeline/workflows")
def list_workflows():
    workflows = []
    if not os.path.isdir(GITHUB_WORKFLOWS):
        return JSONResponse({"workflows": workflows})
    for fname in os.listdir(GITHUB_WORKFLOWS):
        fpath = os.path.join(GITHUB_WORKFLOWS, fname)
        if os.path.isfile(fpath) and (fname.endswith(".yml") or fname.endswith(".yaml")):
            workflows.append(fname)
    return JSONResponse({"workflows": workflows})

@router.post("/api/pipeline/act")
async def run_act(request: Request):
    data = await request.json()
    workflow = data.get("workflow")
    event = data.get("event", "push")
    if not workflow or not workflow.endswith((".yml", ".yaml")):
        return PlainTextResponse("Invalid workflow filename", status_code=400)
    workflow_path = os.path.join(GITHUB_WORKFLOWS, workflow)
    if not os.path.isfile(workflow_path):
        return PlainTextResponse("Workflow not found", status_code=404)

    if not _docker_ready():
        _try_start_docker_desktop()
        # Give Docker a short window to initialize before running act.
        for _ in range(20):
            if _docker_ready():
                break
            time.sleep(1)

    if not _docker_ready():
        return PlainTextResponse(
            "Docker is not available. Tried to start Docker Desktop, but no valid Docker connection was found.",
            status_code=503,
        )

    try:
        # Run act for the selected workflow and event
        cmd = ["act", event, "-W", workflow_path]
        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        return PlainTextResponse(proc.stdout + proc.stderr)
    except Exception as e:
        return PlainTextResponse(f"Error running act: {e}", status_code=500)
