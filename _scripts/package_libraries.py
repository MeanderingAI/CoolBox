#!/usr/bin/env python3
"""Build selected libraries and copy their artifacts into lib/packages/<name>/."""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from _scripts.build_scripts.build_deliverable import cmake_build, default_config  # noqa: E402
from deliverable_utils import resolve_libraries  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
LIB_ROOT = ROOT / "lib"
PACKAGE_ROOT = LIB_ROOT / "packages"

_ARTIFACT_SUFFIXES = {".dll", ".lib", ".exe", ".so", ".dylib", ".a"}


def find_artifacts(target_names: list[str]) -> list[Path]:
    if not BUILD_DIR.is_dir():
        return []

    stems = {name.lower() for name in target_names}
    found: list[Path] = []
    for path in BUILD_DIR.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in _ARTIFACT_SUFFIXES:
            continue
        if path.stem.lower() in stems:
            found.append(path)
    return sorted(found)


def package_libraries(names: list[str], package_name: str, config: str | None) -> int:
    try:
        targets = resolve_libraries(names)
    except LookupError as exc:
        print(exc, file=sys.stderr)
        return 1

    code = cmake_build(targets, config)
    if code != 0:
        return code

    artifacts = find_artifacts(targets)
    if not artifacts:
        print("Build succeeded but no matching artifacts were found.", file=sys.stderr)
        return 1

    out_dir = PACKAGE_ROOT / package_name
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    for artifact in artifacts:
        dest = out_dir / artifact.name
        shutil.copy2(artifact, dest)
        print(f"  packaged {artifact.relative_to(ROOT)} -> {dest.relative_to(ROOT)}")

    print(f"Packaged {len(artifacts)} artifact(s) into {out_dir.relative_to(ROOT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build and package selected CoolBox libraries together."
    )
    parser.add_argument(
        "names",
        nargs="+",
        help="Library group names and/or individual library target names.",
    )
    parser.add_argument(
        "--name",
        help="Package directory name (defaults to joined library names).",
    )
    parser.add_argument(
        "--config",
        default=default_config(),
        help="CMake build configuration (Windows defaults to Release).",
    )
    args = parser.parse_args()

    package_name = args.name or "_".join(args.names)
    return package_libraries(args.names, package_name, args.config)


if __name__ == "__main__":
    raise SystemExit(main())
