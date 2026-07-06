# cross_platform_builder

CROSS_PLATFORM_BUILDER for app_builder that generates simulator-oriented CMake configure/build command plans for Android and iOS.

## Build targets

- `cross_platform_builder` (static library)
- `cross_platform_builder_cli` (command plan CLI)
- `cross_platform_builder_tests` (unit tests when `BUILD_TESTING=ON`)

## CLI usage

```bash
./build/OS_GENERICS_cross_platform_builder_build/cross_platform_builder_cli \
  --platform android \
  --project-root . \
  --build-root build \
  --target recording_studio \
  --config Debug
```

```bash
./build/OS_GENERICS_cross_platform_builder_build/cross_platform_builder_cli \
  --platform ios \
  --project-root . \
  --build-root build \
  --target recording_studio \
  --config Debug
```

## Notes

- Android plans default to emulator ABI `x86_64` and API level `24`.
- iOS plans default to generator `Xcode`, sysroot `iphonesimulator`, and arch `arm64`.
- The module generates command plans; it does not directly execute simulator launch steps.
