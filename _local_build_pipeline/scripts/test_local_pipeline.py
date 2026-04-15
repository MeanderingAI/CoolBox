#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List


REPO_ROOT = Path(__file__).resolve().parents[2]
PIPELINE_ROOT = REPO_ROOT / "_local_build_pipeline"
OUT_DIR = PIPELINE_ROOT / "out" / "test-local-pipeline"
WORKFLOW = "build-libs.yaml"
BACKENDS = ("docker-linux", "native-windows", "native-macos")


@dataclass
class BackendResult:
    backend: str
    status: str
    exit_code: int
    message: str


def detect_host() -> str:
    system = platform.system().lower()
    if system.startswith("win"):
        return "windows"
    if system == "darwin":
        return "macos"
    return "linux"


def host_supports_backend(host: str, backend: str) -> bool:
    if backend == "docker-linux":
        return True
    if backend == "native-windows":
        return host == "windows"
    if backend == "native-macos":
        return host == "macos"
    return False


def get_command(host: str, backend: str) -> List[str]:
    if host == "windows":
        return [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(PIPELINE_ROOT / "run_workflow.ps1"),
            "-Workflow",
            WORKFLOW,
            "-Backend",
            backend,
        ]

    return [
        "bash",
        str(PIPELINE_ROOT / "scripts" / "run_workflow.sh"),
        WORKFLOW,
        "--backend",
        backend,
    ]


def ensure_output_dir() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)


def write_summary(host: str, results: List[BackendResult]) -> Path:
    summary_path = OUT_DIR / "summary.txt"
    with summary_path.open("w", encoding="utf-8") as handle:
        handle.write(f"workflow={WORKFLOW}\n")
        handle.write(f"host={host}\n")
        for result in results:
            handle.write(
                f"backend={result.backend} status={result.status} exit_code={result.exit_code} message={result.message}\n"
            )
    return summary_path


def run_backend(host: str, backend: str) -> BackendResult:
    if not host_supports_backend(host, backend):
        return BackendResult(
            backend=backend,
            status="unsupported",
            exit_code=2,
            message=f"{backend} requires a different host platform",
        )

    command = get_command(host, backend)
    log_path = OUT_DIR / f"{backend}.log"
    env = os.environ.copy()

    with log_path.open("w", encoding="utf-8") as log_handle:
        log_handle.write(f"cwd={REPO_ROOT}\n")
        log_handle.write(f"command={' '.join(command)}\n\n")
        log_handle.flush()

        completed = subprocess.run(
            command,
            cwd=REPO_ROOT,
            env=env,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )

    if completed.returncode == 0:
        return BackendResult(
            backend=backend,
            status="passed",
            exit_code=0,
            message=f"see {log_path.relative_to(REPO_ROOT)}",
        )

    return BackendResult(
        backend=backend,
        status="failed",
        exit_code=completed.returncode,
        message=f"see {log_path.relative_to(REPO_ROOT)}",
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the local build-libs pipeline across supported local backends."
    )
    parser.add_argument(
        "--backend",
        action="append",
        choices=BACKENDS,
        dest="backends",
        help="Run only the specified backend. Can be repeated.",
    )
    parser.add_argument(
        "--fail-on-unsupported",
        action="store_true",
        help="Treat unsupported host/backend combinations as failures.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    ensure_output_dir()

    host = detect_host()
    backends = args.backends or list(BACKENDS)
    results: List[BackendResult] = []

    try:
        for backend in backends:
            print(f"Running {WORKFLOW} on backend {backend}...")
            try:
                result = run_backend(host, backend)
            except Exception as error:  # pragma: no cover - defensive wrapper
                result = BackendResult(
                    backend=backend,
                    status="failed",
                    exit_code=1,
                    message=str(error),
                )

            results.append(result)
            write_summary(host, results)
            print(
                f"  - {result.backend}: {result.status}"
                f" (exit={result.exit_code}) {result.message}"
            )
    finally:
        summary_path = write_summary(host, results)

    print(f"Local pipeline summary for {WORKFLOW} on {host}:")
    for result in results:
        print(
            f"  - {result.backend}: {result.status}"
            f" (exit={result.exit_code}) {result.message}"
        )
    print(f"Summary written to {summary_path.relative_to(REPO_ROOT)}")

    has_failure = any(result.status == "failed" for result in results)
    has_unsupported = any(result.status == "unsupported" for result in results)

    if has_failure:
        return 1
    if args.fail_on_unsupported and has_unsupported:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())