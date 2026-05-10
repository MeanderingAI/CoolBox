# OS-invariant Go installer script for CoolBox
# Usage: Run this script in PowerShell (Windows) or bash (Linux/macOS)

import sys
import os
import platform
import shutil
import urllib.request
import tarfile
import zipfile
import subprocess

def is_windows():
    return platform.system() == "Windows"

def is_mac():
    return platform.system() == "Darwin"

def is_linux():
    return platform.system() == "Linux"

def go_version():
    # Try to read go.mod in go_bindings for required Go version
    gomod = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "_libraries", "go_bindings", "go.mod"))
    if os.path.isfile(gomod):
        with open(gomod, "r") as f:
            for line in f:
                if line.strip().startswith("go "):
                    return line.strip().split()[1]
    # Fallback
    return "1.22.3"

def go_url():
    base = f"https://go.dev/dl/go{go_version()}"
    if is_windows():
        return base + ".windows-amd64.zip"
    elif is_mac():
        return base + ".darwin-amd64.tar.gz"
    elif is_linux():
        return base + ".linux-amd64.tar.gz"
    else:
        raise Exception("Unsupported OS")

def go_archive_name():
    return go_url().split("/")[-1]

def go_install_dir():
    # Use _local_build_pipeline/tmp/go for isolation
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "_local_build_pipeline", "tmp", "go"))

def download_go():
    url = go_url()
    archive = go_archive_name()
    print(f"Downloading {url} ...")
    urllib.request.urlretrieve(url, archive)
    return archive

def extract_go(archive, dest):
    print(f"Extracting {archive} to {dest} ...")
    if archive.endswith(".zip"):
        with zipfile.ZipFile(archive, 'r') as z:
            z.extractall(dest)
    elif archive.endswith(".tar.gz"):
        with tarfile.open(archive, 'r:gz') as t:
            t.extractall(dest)
    else:
        raise Exception("Unknown archive format")

def prepend_path(path):
    os.environ["PATH"] = path + os.pathsep + os.environ["PATH"]
    print(f"Prepended {path} to PATH")

def main():
    dest = go_install_dir()
    if os.path.exists(dest):
        print(f"Removing existing Go install at {dest}")
        shutil.rmtree(dest)
    archive = download_go()
    extract_go(archive, os.path.dirname(dest))
    go_bin = os.path.join(dest, "go", "bin") if not is_windows() else os.path.join(dest, "go", "bin")
    prepend_path(go_bin)
    print("Testing Go installation...")
    try:
        subprocess.check_call([os.path.join(go_bin, "go"), "version"])
    except Exception as e:
        print(f"Go install failed: {e}")
        sys.exit(1)
    print("Go installed successfully.")
    os.remove(archive)

if __name__ == "__main__":
    main()
