#!/usr/bin/env bash

set -euo pipefail

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "run_linux.sh only supports Linux." >&2
    exit 1
fi

for command in cmake c++; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Missing required command: $command" >&2
        echo "On Ubuntu/Debian, install the simulator dependencies with:" >&2
        echo "  sudo apt-get install build-essential cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev" >&2
        exit 1
    fi
done

required_headers=(
    X11/Xlib.h
    X11/extensions/Xrandr.h
    X11/extensions/Xinerama.h
    X11/Xcursor/Xcursor.h
    X11/extensions/XInput2.h
    GL/gl.h
)

missing_headers=()
for header in "${required_headers[@]}"; do
    if ! printf '#include <%s>\n' "$header" | c++ -E -x c++ - >/dev/null 2>&1; then
        missing_headers+=("$header")
    fi
done

if (( ${#missing_headers[@]} > 0 )); then
    echo "Missing Linux graphics development headers:" >&2
    printf '  %s\n' "${missing_headers[@]}" >&2
    echo "On Ubuntu/Debian, install them with:" >&2
    echo "  sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev" >&2
    exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
build_dir="${COOLBOX_BUILD_DIR:-"$repo_root/build"}"

cmake -S "$repo_root" -B "$build_dir" \
    -DBUILD_ROBOT_SIMULATOR=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release --target robot_simulator --parallel

executable="$build_dir/_deliverables/apps/robot_simulator/robot_simulator"
if [[ ! -x "$executable" ]]; then
    echo "The robot_simulator build completed but no executable was found at $executable" >&2
    exit 1
fi

exec "$executable" "$@"
