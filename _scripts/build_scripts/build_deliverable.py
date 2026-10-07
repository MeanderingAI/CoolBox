#!/usr/bin/env python3
"""Build CoolBox deliverable apps, library groups, or individual libraries."""

from __future__ import annotations

import argparse
import platform
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from deliverable_utils import resolve_app, resolve_libraries  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"


def default_config() -> str | None:
    return "Release" if platform.system() == "Windows" else None


def cmake_build(targets: list[str], config: str | None) -> int:
    if not BUILD_DIR.is_dir():
        print("Build directory missing. Run 'make configure' first.", file=sys.stderr)
        return 1

    cmd = ["cmake", "--build", str(BUILD_DIR)]
    if config:
        cmd.extend(["--config", config])
    for target in targets:
        cmd.extend(["--target", target])

    print(f"Building {len(targets)} target(s): {', '.join(targets)}")
    result = subprocess.run(cmd, cwd=ROOT)
    return result.returncode


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build CoolBox deliverable apps or libraries."
    )
    parser.add_argument(
        "kind",
        choices=("app", "library", "libraries"),
        help="Deliverable kind to build.",
    )
    parser.add_argument(
        "names",
        nargs="+",
        help="App name, library group name, or individual library target names.",
    )
    parser.add_argument(
        "--config",
        default=default_config(),
        help="CMake build configuration (Windows defaults to Release).",
    )
    args = parser.parse_args()

    try:
        if args.kind == "app":
            if len(args.names) != 1:
                print("Build one app at a time: make build app <name>", file=sys.stderr)
                return 1
            targets = resolve_app(args.names[0])
        else:
            targets = resolve_libraries(args.names)
    except LookupError as exc:
        print(exc, file=sys.stderr)
        return 1

    return cmake_build(targets, args.config)


if __name__ == "__main__":
    raise SystemExit(main())
