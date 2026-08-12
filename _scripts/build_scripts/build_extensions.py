#!/usr/bin/env python3
"""Build one or more bindings under _deliverables/libraries/bindings.

Usage:
  python _scripts/build_scripts/build_extensions.py
  python _scripts/build_scripts/build_extensions.py python_bindings
  python _scripts/build_scripts/build_extensions.py --list
  python _scripts/build_scripts/build_extensions.py --target <cmake_target>
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable


def _print(msg: str) -> None:
    print(msg, flush=True)


def _run(cmd: list[str], cwd: Path, env: dict[str, str] | None = None) -> int:
    _print(f"$ {' '.join(cmd)}")
    process = subprocess.Popen(
        cmd,
        cwd=str(cwd),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    assert process.stdout is not None
    for line in process.stdout:
        print(line, end="", flush=True)
    return process.wait()


def _is_binding_dir(path: Path) -> bool:
    return path.is_dir() and not path.name.startswith(".") and not path.name.startswith("_")


def _detect_build_type(binding_dir: Path) -> str:
    name = binding_dir.name.lower()
    if name == "postgres_bindings":
        return "postgres"

    has_cmake = (binding_dir / "CMakeLists.txt").is_file()
    if has_cmake and "emscripten" in name:
        return "emscripten"
    if has_cmake:
        return "cmake"
    if (binding_dir / "Cargo.toml").is_file():
        return "cargo"
    if (binding_dir / "go.mod").is_file():
        return "go"
    if (binding_dir / "pom.xml").is_file():
        return "maven"
    if (binding_dir / "package.json").is_file():
        return "npm"
    if (binding_dir / "setup.py").is_file() or (binding_dir / "pyproject.toml").is_file():
        return "python"
    if (binding_dir / "v.mod").is_file() or any(binding_dir.rglob("*.v")):
        return "vlang"
    if any(binding_dir.rglob("*.c3")):
        return "c3"
    if (binding_dir / "dub.json").is_file() or (binding_dir / "dub.sdl").is_file() or any(binding_dir.rglob("*.d")):
        return "dlang"
    if (binding_dir / "build.zig").is_file() or (binding_dir / "build.zig.zon").is_file() or any(binding_dir.rglob("*.zig")):
        return "zig"
    if (binding_dir / "DESCRIPTION").is_file():
        return "r"
    return "unknown"


def _r_executable() -> str | None:
    for exe in ("R", "Rscript"):
        found = shutil.which(exe)
        if found:
            return found
    return None


def _build_cmake(binding_dir: Path, target: str | None = None, emscripten: bool = False) -> int:
    build_dir = binding_dir / "build"
    if emscripten and shutil.which("emcmake"):
        code = _run(["emcmake", "cmake", "-S", str(binding_dir), "-B", str(build_dir)], cwd=binding_dir)
    else:
        code = _run(["cmake", "-S", str(binding_dir), "-B", str(build_dir)], cwd=binding_dir)
    if code != 0:
        return code

    build_cmd = ["cmake", "--build", str(build_dir)]
    if target:
        build_cmd.extend(["--target", target])
    return _run(build_cmd, cwd=binding_dir)


def _build_python(binding_dir: Path) -> int:
    build_sh = binding_dir / "build.sh"
    if build_sh.is_file():
        return _run(["bash", str(build_sh)], cwd=binding_dir)
    setup_py = binding_dir / "setup.py"
    if setup_py.is_file():
        return _run([sys.executable, "setup.py", "build_ext", "--inplace"], cwd=binding_dir)
    return _run([sys.executable, "-m", "pip", "install", "-e", "."], cwd=binding_dir)


def _build_npm(binding_dir: Path) -> int:
    package_json = binding_dir / "package.json"
    if not package_json.is_file():
        return 1

    install_cmd = ["npm", "ci"] if (binding_dir / "package-lock.json").is_file() else ["npm", "install"]
    code = _run(install_cmd, cwd=binding_dir)
    if code != 0:
        return code

    scripts: dict[str, object] = {}
    try:
        scripts = json.loads(package_json.read_text(encoding="utf-8")).get("scripts", {}) or {}
    except Exception:
        scripts = {}

    if "build" in scripts:
        return _run(["npm", "run", "build"], cwd=binding_dir)

    _print("No npm build script found; dependencies installed.")
    return 0


def _build_c3(binding_dir: Path) -> int:
    c3_files = sorted(binding_dir.rglob("*.c3"))
    if not c3_files:
        _print("No .c3 files found.")
        return 1

    project_files = sorted(binding_dir.rglob("*.c3l"))
    if project_files:
        return _run(["c3c", "build", str(project_files[0])], cwd=binding_dir)

    return _run(["c3c", "compile", str(c3_files[0])], cwd=binding_dir)


def _build_dlang(binding_dir: Path) -> int:
    if (binding_dir / "dub.json").is_file() or (binding_dir / "dub.sdl").is_file():
        return _run(["dub", "build"], cwd=binding_dir)

    d_files = sorted(binding_dir.rglob("*.d"))
    if not d_files:
        _print("No D source files found.")
        return 1

    compiler = shutil.which("dmd") or shutil.which("ldc2")
    if not compiler:
        _print("Neither dmd nor ldc2 found.")
        return 1

    return _run([compiler, "-c", str(d_files[0])], cwd=binding_dir)


def _build_zig(binding_dir: Path) -> int:
    if not (binding_dir / "build.zig").is_file():
        _print("build.zig not found.")
        return 1
    return _run(["zig", "build"], cwd=binding_dir)


def _build_r(binding_dir: Path) -> int:
    r_exe = _r_executable()
    if not r_exe:
        _print("R or Rscript not found in PATH.")
        return 1

    if Path(r_exe).name.lower() == "rscript":
        r_cmd = str(Path(r_exe).with_name("R"))
        if os.name == "nt":
            r_cmd += ".exe"
    else:
        r_cmd = r_exe

    return _run([r_cmd, "CMD", "build", str(binding_dir)], cwd=binding_dir)


def _build_postgres(binding_dir: Path) -> int:
    sql_dir = binding_dir / "sql"
    if not sql_dir.is_dir():
        _print("postgres_bindings/sql directory not found.")
        return 1

    sql_files = sorted(sql_dir.glob("*.sql"))
    if not sql_files:
        _print("No SQL files found for postgres bindings.")
        return 1

    _print(f"Validated {len(sql_files)} SQL file(s) by presence check.")

    dsn = os.environ.get("POSTGRES_DSN", "").strip()
    psql = shutil.which("psql")
    if dsn and psql:
        _print("POSTGRES_DSN and psql detected; running live SQL validation.")
        exit_code = 0
        for sql_file in sql_files:
            code = _run([psql, dsn, "-v", "ON_ERROR_STOP=1", "-f", str(sql_file)], cwd=binding_dir)
            if code != 0:
                exit_code = code
                break
        return exit_code

    if dsn and not psql:
        _print("POSTGRES_DSN set but psql is missing; skipping live validation.")
    return 0


def _build_binding(binding_dir: Path, build_type: str, cmake_target: str | None) -> int:
    if build_type == "cmake":
        return _build_cmake(binding_dir, target=cmake_target, emscripten=False)
    if build_type == "emscripten":
        return _build_cmake(binding_dir, target=cmake_target, emscripten=True)
    if build_type == "cargo":
        return _run(["cargo", "build"], cwd=binding_dir)
    if build_type == "go":
        return _run(["go", "build", "./..."], cwd=binding_dir)
    if build_type == "maven":
        return _run(["mvn", "-B", "package", "-DskipTests"], cwd=binding_dir)
    if build_type == "npm":
        return _build_npm(binding_dir)
    if build_type == "python":
        return _build_python(binding_dir)
    if build_type == "vlang":
        return _run(["v", "test", "."], cwd=binding_dir)
    if build_type == "c3":
        return _build_c3(binding_dir)
    if build_type == "dlang":
        return _build_dlang(binding_dir)
    if build_type == "zig":
        return _build_zig(binding_dir)
    if build_type == "r":
        return _build_r(binding_dir)
    if build_type == "postgres":
        return _build_postgres(binding_dir)

    _print(f"Skipping {binding_dir.name}: unknown build type.")
    return 0


def _iter_bindings(bindings_root: Path) -> Iterable[Path]:
    for path in sorted(bindings_root.iterdir()):
        if _is_binding_dir(path):
            yield path


def main() -> int:
    parser = argparse.ArgumentParser(description="Build CoolBox bindings.")
    parser.add_argument("binding", nargs="?", default="", help="Binding folder name, e.g. python_bindings")
    parser.add_argument("--list", action="store_true", dest="list_only", help="List available bindings and build types")
    parser.add_argument("--target", default="", help="Override cmake target for cmake/emscripten bindings")
    args = parser.parse_args()

    script_path = Path(__file__).resolve()
    repo_root = script_path.parents[2]
    bindings_root = repo_root / "_deliverables" / "libraries" / "bindings"

    if not bindings_root.is_dir():
        _print(f"Bindings directory not found: {bindings_root}")
        return 1

    all_bindings = list(_iter_bindings(bindings_root))
    if args.list_only:
        for binding_dir in all_bindings:
            _print(f"{binding_dir.name}: {_detect_build_type(binding_dir)}")
        return 0

    targets: list[Path]
    if args.binding:
        target_dir = bindings_root / args.binding
        if not _is_binding_dir(target_dir):
            _print(f"Binding not found: {args.binding}")
            return 1
        targets = [target_dir]
    else:
        targets = all_bindings

    if not targets:
        _print("No bindings found.")
        return 0

    cmake_target = args.target.strip() or None
    failures: list[str] = []

    for binding_dir in targets:
        build_type = _detect_build_type(binding_dir)
        _print(f"\n=== Building {binding_dir.name} ({build_type}) ===")
        code = _build_binding(binding_dir, build_type, cmake_target)
        if code != 0:
            failures.append(binding_dir.name)
            _print(f"FAILED: {binding_dir.name} (exit {code})")
        else:
            _print(f"OK: {binding_dir.name}")

    if failures:
        _print("\nBuild finished with failures:")
        for name in failures:
            _print(f"- {name}")
        return 1

    _print("\nBuild finished successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
