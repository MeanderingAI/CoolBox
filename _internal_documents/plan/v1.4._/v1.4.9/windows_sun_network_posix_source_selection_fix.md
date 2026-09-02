# v1.4.9 Windows SUN Network POSIX Source Selection Fix

## Issue

Windows CI failed while building `sun_network` with:

- `#error "sun_network currently supports POSIX sockets (macOS/Linux)."`
- `fatal error: arpa/inet.h: No such file or directory`

Failing file:

- `_deliverables/libraries/groups/SUN/COMMS/NETWORK/source/sun_pooled_sql_server.cpp`

## Root cause

The `sun_network` target always compiled the POSIX implementation source, even on Windows.
That source intentionally uses POSIX-only headers (`arpa/inet.h`, `netinet/in.h`, `sys/socket.h`, `unistd.h`) and has an explicit Windows `#error` guard.

## Fix

Updated CMake source selection in:

- `_deliverables/libraries/groups/SUN/COMMS/NETWORK/CMakeLists.txt`

Behavior now:

- `WIN32`: builds `source/sun_pooled_sql_server_windows_stub.cpp`
- non-Windows: builds `source/sun_pooled_sql_server.cpp`

Added Windows stub implementation:

- `_deliverables/libraries/groups/SUN/COMMS/NETWORK/source/sun_pooled_sql_server_windows_stub.cpp`

Stub behavior:

- preserves public API
- `run()` returns `false` (feature unavailable on Windows in current implementation)
- `stop()` is a safe no-op state transition

## Validation

Local non-Windows regression check passed:

- `cmake --build build --target sun_network -j4`
- `BUILD_EXIT_CODE:0`

Windows CI is expected to stop compiling POSIX-only socket code for `sun_network` and therefore avoid the previous build failure.

## Impact

- Restores Windows build compatibility for `sun_network` target.
- No behavior changes for macOS/Linux implementation paths.
