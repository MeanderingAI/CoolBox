#!/usr/bin/env bash
# install_doxygen_linux.sh
# Install Doxygen on Linux using the available package manager.
# Called by _scripts/script_library_runner.py when doxygen is not on PATH.

set -euo pipefail

TAG="[install_doxygen]"

# Use sudo only when not already root
SUDO=""
if command -v sudo &>/dev/null && [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

echo "$TAG Detecting package manager..."

if command -v apt-get &>/dev/null; then
    echo "$TAG Using apt-get..."
    $SUDO apt-get update -qq
    $SUDO apt-get install -y --no-install-recommends doxygen

elif command -v dnf &>/dev/null; then
    echo "$TAG Using dnf..."
    $SUDO dnf install -y doxygen

elif command -v yum &>/dev/null; then
    echo "$TAG Using yum..."
    $SUDO yum install -y doxygen

elif command -v pacman &>/dev/null; then
    echo "$TAG Using pacman..."
    $SUDO pacman -Sy --noconfirm doxygen

elif command -v zypper &>/dev/null; then
    echo "$TAG Using zypper (openSUSE)..."
    $SUDO zypper install -y doxygen

elif command -v apk &>/dev/null; then
    echo "$TAG Using apk (Alpine)..."
    $SUDO apk add --no-cache doxygen

elif command -v brew &>/dev/null; then
    # Linuxbrew fallback
    echo "$TAG Using Homebrew (Linuxbrew)..."
    brew install doxygen

elif command -v snap &>/dev/null; then
    echo "$TAG Using snap..."
    $SUDO snap install doxygen

else
    echo "$TAG ERROR: No supported package manager found." >&2
    echo "$TAG Please install Doxygen manually: https://www.doxygen.nl/download.html" >&2
    exit 1
fi

if command -v doxygen &>/dev/null; then
    echo "$TAG Doxygen $(doxygen --version) installed successfully."
else
    echo "$TAG Warning: 'doxygen' not found on PATH after install." >&2
    echo "$TAG You may need to open a new shell or update your PATH." >&2
    exit 1
fi
