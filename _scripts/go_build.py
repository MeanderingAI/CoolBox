# Go build helper for CoolBox Go bindings
# This script ensures the workspace-local Go (installed by install_go.py) is used for all build/test commands.
# Usage: python _scripts/go_build.py [go arguments]


import os
import sys
import subprocess
import platform

# Path to workspace-local Go binary
GO_BIN = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '_local_build_pipeline', 'tmp', 'go', 'bin', 'go.exe' if platform.system() == 'Windows' else 'go'))

if not os.path.isfile(GO_BIN):
    print(f"Go binary not found at {GO_BIN}. Please run _scripts/install_go.py first.")
    sys.exit(1)

# Check for coolboxbridge shared library
if platform.system() == 'Windows':
    libname = 'coolboxbridge.dll'
elif platform.system() == 'Darwin':
    libname = 'libcoolboxbridge.dylib'
else:
    libname = 'libcoolboxbridge.so'

# Standard search locations
search_dirs = [
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '_libraries', 'go_bindings', 'cbridge', 'build', 'Release')),
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '_libraries', 'go_bindings', 'cbridge', 'build')),
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '_libraries', 'go_bindings', 'cbridge', 'build', 'x64', 'Release')),
]


found = False
for d in search_dirs:
    if os.path.isfile(os.path.join(d, libname)):
        found = True
        break

if not found:
    print(f"{libname} not found. Attempting to build the C++ bridge...")
    build_cmd = [
        'cmake', '--build', os.path.join(os.path.dirname(__file__), '..', '_libraries', 'go_bindings', 'cbridge', 'build'),
        '--config', 'Release', '--target', 'coolboxbridge', '--clean-first'
    ]
    print(' '.join(build_cmd))
    result = subprocess.run(build_cmd)
    # Re-check for the DLL/SO/DYLIB
    found = False
    for d in search_dirs:
        if os.path.isfile(os.path.join(d, libname)):
            found = True
            break
    if not found:
        print(f"ERROR: {libname} still not found after build attempt.")
        sys.exit(2)

# Set GOTOOLCHAIN=local to avoid toolchain downloads
os.environ['GOTOOLCHAIN'] = 'local'

# Default: run in go_bindings module directory
GO_BINDINGS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '_libraries', 'go_bindings'))
os.chdir(GO_BINDINGS_DIR)

# Pass through all arguments to go
args = [GO_BIN] + sys.argv[1:]

print(f"Running: {' '.join(args)} (cwd={os.getcwd()})")

try:
    result = subprocess.run(args, check=True)
    sys.exit(result.returncode)
except subprocess.CalledProcessError as e:
    sys.exit(e.returncode)
