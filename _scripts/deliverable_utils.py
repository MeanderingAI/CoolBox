#!/usr/bin/env python3
"""Shared helpers for listing and building CoolBox deliverable apps and libraries."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DELIVERABLES = ROOT / "_deliverables"
APPS_ROOT = DELIVERABLES / "apps"
LIBRARIES_ROOT = DELIVERABLES / "libraries"

TARGET_RE = re.compile(
    r"\b(add_executable|add_library|add_custom_target)\s*\(\s*([\w.\-]+)",
    re.IGNORECASE,
)
TEST_TARGET_RE = re.compile(r"(^test_|_tests?$)", re.IGNORECASE)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def strip_cmake_comments(text: str) -> str:
    return "\n".join(line.split("#", 1)[0] for line in text.splitlines())


def cmake_targets(path: Path, kinds: set[str] | None = None) -> list[str]:
    text = strip_cmake_comments(read_text(path))
    targets: list[str] = []
    for match in TARGET_RE.finditer(text):
        kind, name = match.groups()
        if kinds is None or kind.lower() in kinds:
            targets.append(name)
    return sorted(dict.fromkeys(targets))


def iter_app_dirs() -> list[Path]:
    if not APPS_ROOT.is_dir():
        return []
    return sorted(
        p for p in APPS_ROOT.iterdir()
        if p.is_dir() and not p.name.startswith((".", "_"))
    )


def app_targets(app_dir: Path) -> list[str]:
    cmake = app_dir / "CMakeLists.txt"
    if not cmake.is_file():
        return []
    return cmake_targets(cmake, {"add_executable"})


def iter_library_cmake_files() -> list[Path]:
    if not LIBRARIES_ROOT.is_dir():
        return []
    return sorted(LIBRARIES_ROOT.rglob("CMakeLists.txt"))


def library_targets_for_cmake(cmake: Path) -> list[str]:
    return [
        target
        for target in cmake_targets(cmake, {"add_library"})
        if not TEST_TARGET_RE.search(target)
    ]


def all_library_entries() -> list[tuple[Path, list[str]]]:
    rows: list[tuple[Path, list[str]]] = []
    for cmake in iter_library_cmake_files():
        targets = library_targets_for_cmake(cmake)
        if targets:
            rows.append((cmake.parent, targets))
    return rows


def resolve_app(name: str) -> list[str]:
    matches = [d for d in iter_app_dirs() if d.name == name]
    if not matches:
        raise LookupError(f"Unknown app '{name}'. Run: make list_apps")

    targets: list[str] = []
    for app_dir in matches:
        targets.extend(app_targets(app_dir))
    targets = sorted(dict.fromkeys(targets))
    if not targets:
        raise LookupError(f"App '{name}' has no CMake executable targets.")
    return targets


def app_dir_for(name: str) -> Path:
    matches = [d for d in iter_app_dirs() if d.name == name]
    if not matches:
        raise LookupError(f"Unknown app '{name}'. Run: make list_apps")
    return matches[0]


def app_python_scripts(app_dir: Path) -> list[str]:
    return sorted(
        p.name
        for p in app_dir.iterdir()
        if p.is_file() and p.suffix == ".py" and not p.name.startswith("_")
    )


def resolve_app_run_target(app_name: str, target: str | None = None) -> str:
    """Pick the cmake executable target to run for an app."""
    targets = resolve_app(app_name)
    if target:
        if target not in targets:
            raise LookupError(
                f"App '{app_name}' has no executable target '{target}'. "
                f"Available: {', '.join(targets)}"
            )
        return target
    if app_name in targets:
        return app_name
    if len(targets) == 1:
        return targets[0]
    raise LookupError(
        f"App '{app_name}' has multiple executables: {', '.join(targets)}. "
        f"Specify one: make run app {app_name} <target>"
    )


def _path_matches_library_name(path: Path, name: str) -> bool:
    return name in path.relative_to(ROOT).parts


def resolve_libraries(names: list[str]) -> list[str]:
    if not names:
        raise LookupError("Provide at least one library or group name.")

    entries = all_library_entries()
    known_targets = {target for _, targets in entries for target in targets}
    resolved: list[str] = []

    for name in names:
        by_path = [
            target
            for cmake_dir, targets in entries
            if _path_matches_library_name(cmake_dir, name)
            for target in targets
        ]
        if by_path:
            resolved.extend(by_path)
            continue

        if name in known_targets:
            resolved.append(name)
            continue

        case_matches = [t for t in known_targets if t.lower() == name.lower()]
        if case_matches:
            resolved.extend(case_matches)
            continue

        raise LookupError(
            f"Unknown library or group '{name}'. Run: make list_libraries"
        )

    return sorted(dict.fromkeys(resolved))
