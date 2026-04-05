Build instructions for the Go bindings bridge library

This package separates the heavy C++ bridge into `cbridge/` which builds
a static `coolboxbridge` archive that the Go package links against.
The Go package root must not contain standalone `.cpp` bridge sources;
those belong under `cbridge/` so `go test` can treat the package as a cgo
consumer instead of a plain Go package.

Build the bridge (recommended):

```sh
cd _libraries/go_bindings/cbridge
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target coolboxbridge -- -j
```

After building, return to the Go package root and run tests/builds:

```sh
cd ..
# Ensure CGO is enabled (default on most systems where a C++ compiler exists)
export CGO_ENABLED=1
go test ./...
```

Notes:
- On Windows, build the `cbridge` target with the Visual Studio generator:
  `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` then
  `cmake --build build --config Release --target coolboxbridge`.
- The `cbridge/bridge.cpp` file textually includes several repository source files
  (as before) so the CMake target needs access to the repository include
  directories; the provided `CMakeLists.txt` contains common include
  paths but you may need to adjust them if your build tree is different.
