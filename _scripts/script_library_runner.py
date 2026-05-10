#!/usr/bin/env python3
"""
script_library_runner.py
========================
Selects and runs the correct install script for a given tool based on the
current platform, then verifies the tool is available after install.

Script naming convention (in _scripts/install_scripts/):
    install_<tool>_windows.ps1
    install_<tool>_linux.sh
    install_<tool>_macos.sh

Usage
-----
    python _scripts/script_library_runner.py --tool doxygen
    python _scripts/script_library_runner.py --tool doxygen --os linux
    python _scripts/script_library_runner.py --list-tools
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


# ── Configuration ─────────────────────────────────────────────────────────────

SCRIPTS_DIR = Path(__file__).parent / "install_scripts"

# Map platform.system() → canonical OS key used in script filenames
PLATFORM_MAP: dict[str, str] = {
    "Windows": "windows",
    "Linux":   "linux",
    "Darwin":  "macos",
}

# How to invoke each OS script type
def _ps1_cmd(path: Path) -> list[str]:
    return [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", str(path),
    ]

def _sh_cmd(path: Path) -> list[str]:
    return ["bash", str(path)]

RUNNERS: dict[str, dict] = {
    "windows": {"ext": ".ps1", "cmd": _ps1_cmd},
    "linux":   {"ext": ".sh",  "cmd": _sh_cmd},
    "macos":   {"ext": ".sh",  "cmd": _sh_cmd},
}


# ── Helpers ───────────────────────────────────────────────────────────────────

def _log(msg: str) -> None:
    print(f"[runner] {msg}", flush=True)


def _detect_os() -> str:
    system = platform.system()
    os_name = PLATFORM_MAP.get(system)
    if os_name is None:
        print(f"[runner] Unsupported platform: {system!r}", file=sys.stderr)
        sys.exit(1)
    return os_name


def _find_script(tool: str, os_name: str) -> Path | None:
    """Return the path to install_<tool>_<os>.<ext>, or None if not found."""
    ext = RUNNERS[os_name]["ext"]
    candidate = SCRIPTS_DIR / f"install_{tool}_{os_name}{ext}"
    return candidate if candidate.is_file() else None


def _list_tools() -> None:
    """Print all tools that have at least one install script."""
    if not SCRIPTS_DIR.is_dir():
        print(f"[runner] install_scripts/ not found at {SCRIPTS_DIR}", file=sys.stderr)
        sys.exit(1)

    tools: set[str] = set()
    known_os = set(RUNNERS.keys())
    for f in SCRIPTS_DIR.iterdir():
        stem = f.stem  # e.g. "install_doxygen_linux"
        parts = stem.split("_")
        # Expected: ["install", <tool...>, <os>]
        if len(parts) >= 3 and parts[0] == "install" and parts[-1] in known_os:
            tool_name = "_".join(parts[1:-1])
            tools.add(tool_name)

    if tools:
        print("Available tools:")
        for t in sorted(tools):
            available_on = []
            for os_name in sorted(RUNNERS):
                if _find_script(t, os_name):
                    available_on.append(os_name)
            print(f"  {t:<20} [{', '.join(available_on)}]")
    else:
        print("[runner] No install scripts found in", SCRIPTS_DIR)


def _run_install(tool: str, os_name: str) -> int:
    """Find, run, and report on the install script. Returns exit code."""
    script = _find_script(tool, os_name)
    if script is None:
        ext = RUNNERS[os_name]["ext"]
        expected = SCRIPTS_DIR / f"install_{tool}_{os_name}{ext}"
        print(
            f"[runner] No install script found for tool={tool!r} os={os_name!r}.\n"
            f"         Expected: {expected}\n"
            f"         Run --list-tools to see available scripts.",
            file=sys.stderr,
        )
        return 1

    cmd = RUNNERS[os_name]["cmd"](script)
    _log(f"Running: {' '.join(cmd)}")

    result = subprocess.run(cmd)
    rc = result.returncode

    if rc == 0:
        _log(f"Install script exited successfully.")
        # Verify the binary is now detectable
        binary = tool.split("_")[0]  # e.g. "doxygen" from "doxygen"
        found = shutil.which(binary)
        if found:
            _log(f"'{binary}' is now available at: {found}")
        else:
            _log(
                f"'{binary}' not found on PATH yet — you may need to restart "
                f"your terminal or the server process to pick up the new PATH."
            )
    else:
        _log(f"Install script exited with code {rc}.")

    return rc


# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Run the OS-appropriate install script for a tool.\n"
            "Scripts live in _scripts/install_scripts/ and follow the naming\n"
            "convention: install_<tool>_<os>.[ps1|sh]"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--tool",
        metavar="TOOL",
        help="Tool to install, e.g. 'doxygen'",
    )
    parser.add_argument(
        "--os",
        dest="os_override",
        choices=list(RUNNERS.keys()),
        metavar="{" + "|".join(RUNNERS) + "}",
        help="Override OS detection",
    )
    parser.add_argument(
        "--list-tools",
        action="store_true",
        help="List all tools that have install scripts",
    )

    args = parser.parse_args()

    if args.list_tools:
        _list_tools()
        return

    if not args.tool:
        parser.print_help()
        sys.exit(1)

    os_name = args.os_override or _detect_os()
    sys.exit(_run_install(args.tool, os_name))


if __name__ == "__main__":
    main()
