# Quic Transport CI Include Fix

## Summary
- Updated `_libraries/backages/IO/quic_transport/CMakeLists.txt` so the `quic_transport_tests` target explicitly includes the HTTP request/response headers consumed by `quic_transport.hpp`.
- Kept the earlier `quic_transport` library include-path fix in place and extended the same header visibility to the test executable.

## Reason For The Change
- After the initial library-target include fix, Linux CI was able to build `quic_transport` itself but still failed when compiling `quic_transport_tests`.
- The failing test translation unit includes `quic_transport.hpp`, which exposes `Request` and `Response` types from `request_response.h` in its public interface.
- The CI build showed that relying on transitive include propagation was not sufficient for the test target in that configuration.

## CMake Change
- `quic_transport_tests` now adds:
  - `${CMAKE_CURRENT_SOURCE_DIR}/headers`
  - `${CMAKE_CURRENT_SOURCE_DIR}/../dataformats/http/headers`
- This keeps the test target aligned with the public header dependencies already required by the `quic_transport` library.

## Result
- The `quic_transport_tests` target now has direct visibility to `request_response.h`, which removes the Linux CI compile failure that remained after the library-only fix.