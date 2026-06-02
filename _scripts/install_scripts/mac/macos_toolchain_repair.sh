#!/bin/bash
# macos_toolchain_repair.sh
# This script repairs Xcode Command Line Tools and checks for header shadowing issues.
# Usage: sudo ./macos_toolchain_repair.sh

set -e

# 1. Remove old Command Line Tools
if [ -d "/Library/Developer/CommandLineTools" ]; then
  echo "Removing old Command Line Tools..."
  sudo rm -rf /Library/Developer/CommandLineTools
fi

# 2. Reinstall Command Line Tools
if ! xcode-select --print-path &>/dev/null; then
  echo "Installing Xcode Command Line Tools..."
  xcode-select --install || true
  echo "If prompted, follow the GUI to complete installation."
else
  echo "Xcode Command Line Tools already installed."
fi

# 3. Reset Xcode path
sudo xcode-select --reset

# 4. Accept Xcode license
sudo xcodebuild -license accept

# 5. Check for shadowed <complex> headers in project
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
echo "Searching for shadowed 'complex' headers in $PROJECT_ROOT ..."
find "$PROJECT_ROOT" -type f -name 'complex*' | grep -v '/usr/include' || echo "No shadowed complex headers found."

echo "\nToolchain repair steps complete. Please run 'make clean && make build_libraries' again."
