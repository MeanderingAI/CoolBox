#!/usr/bin/env python3

import argparse
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path


WINDOWS_DOWNLOAD_URL = "https://central.github.com/deployments/desktop/desktop/latest/win32"
MACOS_DOWNLOAD_URL = "https://central.github.com/deployments/desktop/desktop/latest/darwin"
SHIFTKEY_RELEASES_API = "https://api.github.com/repos/shiftkey/desktop/releases/latest"


def log(message: str) -> None:
    print(f"[install_gh_desktop] {message}")


def detect_architecture() -> str:
    machine = platform.machine().lower()
    if machine in {"x86_64", "amd64", "x64"}:
        return "x86_64"
    if machine in {"arm64", "aarch64"}:
        return "arm64"
    return machine


def download_file(url: str, destination: Path) -> Path:
    log(f"Downloading {url}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as response, destination.open("wb") as handle:
        shutil.copyfileobj(response, handle)
    return destination


def run_command(command: list[str], dry_run: bool) -> None:
    rendered = " ".join(command)
    log(f"Running: {rendered}")
    if dry_run:
        return
    subprocess.run(command, check=True)


def install_on_windows(temp_dir: Path, dry_run: bool) -> None:
    installer_path = temp_dir / "GitHubDesktopSetup.exe"
    log("Using the official GitHub Desktop Windows installer.")
    if not dry_run:
        download_file(WINDOWS_DOWNLOAD_URL, installer_path)
    else:
        log(f"Would download installer to {installer_path}")
    run_command([str(installer_path)], dry_run)


def resolve_macos_destination() -> Path:
    system_applications = Path("/Applications")
    if os.access(system_applications, os.W_OK):
        return system_applications
    return Path.home() / "Applications"


def install_on_macos(temp_dir: Path, dry_run: bool) -> None:
    archive_path = temp_dir / "GitHubDesktop.zip"
    extract_dir = temp_dir / "GitHubDesktop"
    destination_root = resolve_macos_destination()
    app_destination = destination_root / "GitHub Desktop.app"

    log("Using the official GitHub Desktop macOS archive.")
    if dry_run:
        log(f"Would download archive to {archive_path}")
        log(f"Would extract application bundle into {app_destination}")
        return

    download_file(MACOS_DOWNLOAD_URL, archive_path)
    extract_dir.mkdir(parents=True, exist_ok=True)
    run_command(["ditto", "-xk", str(archive_path), str(extract_dir)], dry_run=False)

    extracted_apps = sorted(extract_dir.rglob("GitHub Desktop.app"))
    if not extracted_apps:
        raise RuntimeError("Unable to locate 'GitHub Desktop.app' in the extracted macOS archive.")

    source_app = extracted_apps[0]
    destination_root.mkdir(parents=True, exist_ok=True)
    if app_destination.exists():
        shutil.rmtree(app_destination)
    shutil.copytree(source_app, app_destination)
    log(f"Installed GitHub Desktop to {app_destination}")


def fetch_latest_shiftkey_release() -> dict:
    with urllib.request.urlopen(SHIFTKEY_RELEASES_API) as response:
        return json.load(response)


def choose_linux_asset(release_payload: dict, architecture: str) -> dict:
    assets = release_payload.get("assets", [])
    appimages = [asset for asset in assets if asset.get("name", "").endswith(".AppImage")]
    if not appimages:
        raise RuntimeError("No AppImage assets were found in the latest shiftkey/desktop release.")

    arch_tokens = {
        "x86_64": ["x86_64", "amd64", "x64"],
        "arm64": ["arm64", "aarch64"],
    }.get(architecture, [architecture])

    for asset in appimages:
        lowered = asset.get("name", "").lower()
        if any(token in lowered for token in arch_tokens):
            return asset

    if len(appimages) == 1:
        return appimages[0]

    available = ", ".join(asset.get("name", "<unknown>") for asset in appimages)
    raise RuntimeError(
        f"Unable to find a Linux AppImage for architecture '{architecture}'. Available assets: {available}"
    )


def install_on_linux(temp_dir: Path, install_dir: Path, dry_run: bool) -> None:
    architecture = detect_architecture()
    log(f"Detected Linux architecture: {architecture}")
    release_payload = fetch_latest_shiftkey_release()
    asset = choose_linux_asset(release_payload, architecture)
    asset_name = asset["name"]
    download_url = asset["browser_download_url"]
    appimage_path = install_dir / asset_name
    launcher_path = install_dir / "github-desktop"

    if dry_run:
        log(f"Would download Linux AppImage {asset_name} to {appimage_path}")
        log(f"Would refresh launcher path {launcher_path}")
        return

    install_dir.mkdir(parents=True, exist_ok=True)
    download_file(download_url, appimage_path)
    current_mode = appimage_path.stat().st_mode
    appimage_path.chmod(current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    if launcher_path.exists() or launcher_path.is_symlink():
        launcher_path.unlink()
    launcher_path.symlink_to(appimage_path.name)
    log(f"Installed Linux AppImage to {appimage_path}")
    log(f"Launcher symlink created at {launcher_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Install GitHub Desktop on Windows, macOS, or Linux from a single cross-platform entry point."
    )
    parser.add_argument(
        "--linux-install-dir",
        default=str(Path.home() / ".local" / "bin" / "github-desktop"),
        help="Linux installation directory for the downloaded AppImage and launcher symlink.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the planned installation steps without making changes.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    system = platform.system()

    log(f"Detected operating system: {system}")
    with tempfile.TemporaryDirectory(prefix="coolbox-gh-desktop-") as temp_root:
        temp_dir = Path(temp_root)
        if system == "Windows":
            install_on_windows(temp_dir=temp_dir, dry_run=args.dry_run)
        elif system == "Darwin":
            install_on_macos(temp_dir=temp_dir, dry_run=args.dry_run)
        elif system == "Linux":
            install_on_linux(
                temp_dir=temp_dir,
                install_dir=Path(args.linux_install_dir).expanduser(),
                dry_run=args.dry_run,
            )
        else:
            raise RuntimeError(f"Unsupported operating system: {system}")

    log("GitHub Desktop installation flow completed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        log(f"ERROR: {error}")
        raise SystemExit(1)