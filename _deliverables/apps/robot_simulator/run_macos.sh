#!/usr/bin/env bash

set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "run_macos.sh only supports macOS." >&2
    exit 1
fi

for command in cmake c++ xcrun; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Missing required command: $command" >&2
        echo "Install Apple's Command Line Tools with: xcode-select --install" >&2
        echo "Install CMake with: brew install cmake" >&2
        exit 1
    fi
done

if ! xcrun --sdk macosx --show-sdk-path >/dev/null 2>&1; then
    echo "The macOS SDK is unavailable. Install Apple's Command Line Tools with: xcode-select --install" >&2
    exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
build_dir="${COOLBOX_BUILD_DIR:-"$repo_root/build"}"

cmake -S "$repo_root" -B "$build_dir" \
    -DBUILD_ROBOT_SIMULATOR=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release --target robot_simulator --parallel

executable="$build_dir/_deliverables/apps/robot_simulator/robot_simulator"
if [[ -x "$build_dir/_deliverables/apps/robot_simulator/Release/robot_simulator" ]]; then
    executable="$build_dir/_deliverables/apps/robot_simulator/Release/robot_simulator"
fi
if [[ ! -x "$executable" ]]; then
    echo "The robot_simulator build completed but no executable was found at $executable" >&2
    exit 1
fi

exec "$executable" "$@"