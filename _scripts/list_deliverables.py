#!/usr/bin/env python3
"""List CoolBox deliverable apps and libraries for Makefile targets."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))


from build_scripts.deliverable_utils import (  # noqa: E402
    ROOT,
    app_targets,
    iter_app_dirs,
    library_targets_for_cmake,
    iter_library_cmake_files,
)


def list_apps() -> int:
    print("Apps in _deliverables/apps")
    print("--------------------------")
    app_dirs = iter_app_dirs()
    if not app_dirs:
        print("(none)")
        return 0

    for app_dir in app_dirs:
        executables = app_targets(app_dir)
        scripts = sorted(p.name for p in app_dir.iterdir() if p.is_file() and p.suffix == ".py")
        traits: list[str] = []
        if executables:
            traits.append("targets: " + ", ".join(executables))
        if scripts:
            traits.append("scripts: " + ", ".join(scripts))
        if (app_dir / "app_page.html").is_file():
            traits.append("app_page")
        print(f"{app_dir.name}: {'; '.join(traits) if traits else 'no buildable targets detected'}")
    return 0


def list_libraries() -> int:
    print("Libraries in _deliverables/libraries")
    print("------------------------------------")
    rows: list[tuple[str, list[str]]] = []
    for cmake in iter_library_cmake_files():
        rel_dir = cmake.parent.relative_to(ROOT).as_posix()
        targets = library_targets_for_cmake(cmake)
        if targets:
            rows.append((rel_dir, targets))

    if not rows:
        print("(none)")
        return 0

    for rel_dir, targets in rows:
        print(f"{rel_dir}: {', '.join(targets)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="List CoolBox deliverable apps or libraries.")
    parser.add_argument("kind", choices=("apps", "libraries"))
    args = parser.parse_args()

    if args.kind == "apps":
        return list_apps()
    return list_libraries()


if __name__ == "__main__":
    raise SystemExit(main())
