#!/usr/bin/env bash
# install_doxygen_macos.sh
# Install Doxygen on macOS using Homebrew (primary) or MacPorts (fallback).
# Called by _scripts/script_library_runner.py when doxygen is not on PATH.

set -euo pipefail

TAG="[install_doxygen]"

if command -v brew &>/dev/null; then
    echo "$TAG Using Homebrew..."
    brew install doxygen

elif command -v port &>/dev/null; then
    echo "$TAG Using MacPorts..."
    sudo port install doxygen

else
    echo "$TAG ERROR: Neither Homebrew nor MacPorts is installed." >&2
    echo "$TAG Install Homebrew from https://brew.sh then re-run this script." >&2
    echo "$TAG Or download Doxygen directly: https://www.doxygen.nl/download.html" >&2
    exit 1
fi

if command -v doxygen &>/dev/null; then
    echo "$TAG Doxygen $(doxygen --version) installed successfully."
else
    echo "$TAG Warning: 'doxygen' not found on PATH after install." >&2
    echo "$TAG Try: brew link doxygen  — or open a new terminal." >&2
    exit 1
fi
