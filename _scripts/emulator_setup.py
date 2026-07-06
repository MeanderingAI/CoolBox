#!/usr/bin/env python3

"""Set up Android and iOS emulators for CoolBox development.

Android setup is available on Linux, macOS, and Windows.
iOS simulator setup is only available on macOS.

By default this script performs a dry run and prints the commands it would
execute. Pass --apply to run the detected setup steps.
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Union


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _system() -> str:
    return platform.system()


def _run(cmd: List[str], *, dry_run: bool) -> int:
    printable = " ".join(cmd)
    print(f"  $ {printable}")
    if dry_run:
        return 0

    completed = subprocess.run(cmd, check=False)
    return completed.returncode


def _run_shell(cmd: str, *, dry_run: bool, cwd: Path | None = None) -> int:
    location = f" (cwd={cwd})" if cwd else ""
    print(f"  $ {cmd}{location}")
    if dry_run:
        return 0

    completed = subprocess.run(cmd, shell=True, cwd=str(cwd) if cwd else None, check=False)
    return completed.returncode


def _which(name: str) -> str | None:
    return shutil.which(name)


@dataclass
class Step:
    name: str
    command: Union[List[str], str]
    shell: bool = False
    cwd: Path | None = None

    def run(self, *, dry_run: bool) -> int:
        print(f"- {self.name}")
        if self.shell:
            assert isinstance(self.command, str)
            return _run_shell(self.command, dry_run=dry_run, cwd=self.cwd)
        assert isinstance(self.command, list)
        return _run(self.command, dry_run=dry_run)


def _android_steps() -> list[Step]:
    steps: list[Step] = []

    sdkmanager = _which("sdkmanager")
    avdmanager = _which("avdmanager")
    emulator = _which("emulator")

    if sdkmanager is None:
        steps.append(
            Step(
                "Android SDK tools not found",
                [
                    "echo",
                    "Install Android Studio or Android SDK command line tools, then re-run this script.",
                ],
            )
        )
        return steps

    assert sdkmanager is not None
    sdk_root = os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME")
    sdk_root_hint = sdk_root if sdk_root else "(set ANDROID_SDK_ROOT or ANDROID_HOME)"

    steps.append(Step("Install Android platform tools", [sdkmanager, "platform-tools"]))
    steps.append(Step("Install Android emulator", [sdkmanager, "emulator"]))
    steps.append(Step("Install Android system image", [sdkmanager, "system-images;android-34;google_apis;x86_64"]))

    avd_name = "CoolBox_API34_x86_64"
    create_avd_cmd = (
        f'"{avdmanager}" create avd -n {avd_name} -k "system-images;android-34;google_apis;x86_64" '
        f"--force"
    )
    if emulator is None:
        steps.append(Step("Android emulator binary not found", ["echo", "Install the Android emulator package."]))
    else:
        steps.append(Step("Create Android virtual device", create_avd_cmd, shell=True))
        steps.append(Step("List available Android emulators", [emulator, "-list-avds"]))
        steps.append(
            Step(
                "Launch Android emulator",
                [emulator, "-avd", avd_name, "-no-snapshot-save", "-wipe-data"],
            )
        )

    if avdmanager is None:
        steps.append(Step("Android avdmanager not found", ["echo", "Install Android command line tools to create AVDs."]))

    steps.append(Step("Android SDK root hint", ["echo", f"ANDROID_SDK_ROOT={sdk_root_hint}"]))
    return steps


def _ios_steps() -> list[Step]:
    steps: list[Step] = []
    if _system() != "Darwin":
        steps.append(Step("iOS simulator unavailable", ["echo", "iOS simulators can only be set up on macOS."]))
        return steps

    xcode_select = _which("xcode-select")
    xcrun = _which("xcrun")

    if xcode_select is None or xcrun is None:
        steps.append(Step("Xcode command line tools not found", ["echo", "Install Xcode and run xcode-select --install."]))
        return steps

    steps.append(Step("Check Xcode command line tools", [xcode_select, "-p"]))
    steps.append(Step("List installed simulator runtimes", [xcrun, "simctl", "list", "runtimes"]))
    steps.append(Step("List available simulator devices", [xcrun, "simctl", "list", "devices"]))

    if xcrun is not None:
        device_name = "CoolBox iPhone 16"
        runtime = "com.apple.CoreSimulator.SimRuntime.iOS-18-0"
        create_cmd = [xcrun, "simctl", "create", device_name, "iPhone 16", runtime]
        boot_cmd = [xcrun, "simctl", "boot", device_name]
        steps.append(Step("Create iOS simulator device", create_cmd))
        steps.append(Step("Boot iOS simulator device", boot_cmd))
    else:
        steps.append(Step("simctl not found", ["echo", "Ensure Xcode command line tools are installed and xcrun is available."]))

    return steps


def _print_plan(steps: Iterable[Step], *, dry_run: bool) -> int:
    for step in steps:
        rc = step.run(dry_run=dry_run)
        if rc != 0:
            return rc
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Set up Android and iOS emulators for CoolBox.")
    parser.add_argument("--android", action="store_true", help="Run Android emulator setup steps.")
    parser.add_argument("--ios", action="store_true", help="Run iOS simulator setup steps.")
    parser.add_argument("--all", action="store_true", help="Run both Android and iOS setup steps.")
    parser.add_argument("--apply", action="store_true", help="Execute the detected commands instead of dry-running them.")

    args = parser.parse_args()
    run_android = args.all or args.android or not args.ios
    run_ios = args.all or args.ios or not args.android

    print("CoolBox emulator setup")
    print(f"Repository root: {_repo_root()}")
    print(f"Host OS: {_system()}")
    print(f"Mode: {'apply' if args.apply else 'dry-run'}")

    steps: list[Step] = []
    if run_android:
        print("\nAndroid")
        steps.extend(_android_steps())
    if run_ios:
        print("\niOS")
        steps.extend(_ios_steps())

    if not steps:
        print("No setup steps selected.")
        return 0

    return _print_plan(steps, dry_run=not args.apply)


if __name__ == "__main__":
    raise SystemExit(main())