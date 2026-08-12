#!/usr/bin/env python3
"""Run a built CoolBox deliverable app."""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from _scripts.build_scripts.build_deliverable import cmake_build, default_config  # noqa: E402
from deliverable_utils import (  # noqa: E402
    ROOT,
    app_dir_for,
    app_python_scripts,
    app_targets,
    resolve_app_run_target,
)

BUILD_DIR = ROOT / "build"


def _config_dirs(config: str | None) -> list[str | None]:
    if platform.system() == "Windows":
        return [config or "Release", "Debug"]
    return [None]


def find_app_executable(app_name: str, target: str, config: str | None) -> Path | None:
    """Return the built executable for an app target, if it exists."""
    names = [target, f"{target}.exe"]
    preferred_roots = [
        BUILD_DIR / "_deliverables" / "apps" / app_name,
        BUILD_DIR / "_deliverables" / "apps",
        BUILD_DIR,
    ]

    for root in preferred_roots:
        if not root.is_dir():
            continue
        for cfg in _config_dirs(config):
            search_root = root / cfg if cfg else root
            if not search_root.is_dir():
                continue
            for path in search_root.rglob("*"):
                if not path.is_file():
                    continue
                if path.name not in names:
                    continue
                if path.suffix.lower() not in {".exe", ""}:
                    continue
                if path.suffix == ".exe" or os.access(path, os.X_OK):
                    return path
    return None


def run_python_app(app_name: str, script: str | None) -> int:
    app_dir = app_dir_for(app_name)
    scripts = app_python_scripts(app_dir)
    if not scripts:
        raise LookupError(f"App '{app_name}' has no Python scripts.")

    script_name = script or scripts[0]
    if script_name not in scripts:
        raise LookupError(
            f"App '{app_name}' has no script '{script_name}'. "
            f"Available: {', '.join(scripts)}"
        )

    script_path = app_dir / script_name
    python_exe = sys.executable
    print(f"Running {script_path.relative_to(ROOT)}")
    return subprocess.run([python_exe, str(script_path)], cwd=app_dir).returncode


def run_cmake_app(
    app_name: str,
    target: str | None,
    config: str | None,
    build_if_missing: bool,
) -> int:
    exe_target = resolve_app_run_target(app_name, target)
    exe_path = find_app_executable(app_name, exe_target, config)

    if exe_path is None and build_if_missing:
        print(f"'{exe_target}' not built yet — building first...")
        code = cmake_build([exe_target], config)
        if code != 0:
            return code
        exe_path = find_app_executable(app_name, exe_target, config)

    if exe_path is None:
        rel = f"build/_deliverables/apps/{app_name}/"
        if platform.system() == "Windows":
            rel += f"{config or 'Release'}/{exe_target}.exe"
        else:
            rel += exe_target
        print(
            f"Built app not found for '{exe_target}'.\n"
            f"Expected under {rel}\n"
            f"Build first: make build app {app_name}",
            file=sys.stderr,
        )
        return 1

    print(f"Running {exe_path.relative_to(ROOT)}")
    env = os.environ.copy()
    lib_dir = ROOT / "lib"
    if lib_dir.is_dir():
        if platform.system() == "Windows":
            env["PATH"] = str(lib_dir) + os.pathsep + env.get("PATH", "")
        elif platform.system() == "Darwin":
            env["DYLD_LIBRARY_PATH"] = str(lib_dir) + os.pathsep + env.get("DYLD_LIBRARY_PATH", "")
        else:
            env["LD_LIBRARY_PATH"] = str(lib_dir) + os.pathsep + env.get("LD_LIBRARY_PATH", "")

    return subprocess.run([str(exe_path)], cwd=exe_path.parent, env=env).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a CoolBox deliverable app.")
    parser.add_argument("app", help="App folder name under _deliverables/apps.")
    parser.add_argument(
        "target",
        nargs="?",
        help="Optional executable or .py script name when an app has several.",
    )
    parser.add_argument(
        "--config",
        default=default_config(),
        help="CMake build configuration (Windows defaults to Release).",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Do not build automatically when the executable is missing.",
    )
    args = parser.parse_args()

    try:
        app_dir = app_dir_for(args.app)
        cmake_targets_list = app_targets(app_dir)
        scripts = app_python_scripts(app_dir)

        if cmake_targets_list:
            return run_cmake_app(
                args.app,
                args.target,
                args.config,
                build_if_missing=not args.no_build,
            )
        if scripts:
            return run_python_app(args.app, args.target)
        raise LookupError(f"App '{args.app}' has no runnable cmake or Python targets.")
    except LookupError as exc:
        print(exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
