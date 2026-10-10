# CoolBox v3.2.6 Release Notes

## Windows Python Packaging

Fixed the Windows Python purchase workflow failure that occurred after the
`charts` native library built successfully:

```text
MSBUILD : error MSB1009: Project file does not exist.
Switch: wave_generator_utils.vcxproj
```

The workflow correctly requested the `wave_generator_utils` target, but the
target's CMake directory was not included by the TREKKER MISC package. As a
result, Visual Studio never generated `wave_generator_utils.vcxproj`.

The TREKKER MISC CMake file now adds the wave-generator package explicitly.
The target is therefore generated and can be built by the Windows Python
packaging step alongside `charts`.

### Configuration Warning Cleanup

The related Windows configuration output is now quieter and uses current
CMake dependency APIs:

- pybind11 was updated from v2.13.6 to v3.0.1, removing its obsolete CMake
  compatibility warning;
- Eigen header population now uses `FetchContent_MakeAvailable` instead of
  deprecated `FetchContent_Populate`, without adding Eigen's tests;
- Windows skips Unix-only pthread, `argon2.h`, `crypt.h`, and `crypt`
  capability probes in the password-hash target;
- the intentionally unavailable `py_circuitry` binding is reported as normal
  status output instead of a warning.

Unix platforms retain their thread, Argon2, and `crypt` provider detection.
Other targets may still show CMake's normal pthread capability checks on
Windows; failed pthread-name probes followed by `Found Threads: TRUE` are
expected MSVC feature detection, not warnings or build failures.

## Verification

- [x] Confirmed `wave_generator_utils` is defined in the wave-generator
  package.
- [x] Confirmed the parent TREKKER MISC CMake file omitted that package.
- [x] Added the missing `add_subdirectory(wave_generator)` declaration.
- [x] Removed deprecated Eigen population calls from native and Emscripten
  dependency setup.
- [x] Removed irrelevant Unix capability probes from Windows configuration.
- [ ] Rerun the Windows Python purchase workflow and confirm both
  `charts.lib` and `wave_generator_utils` build successfully.
