# CoolBox v3.2.0 Release Notes

## Robot Simulator Run and Debug Integration

Added a Windows Run and Debug configuration for the optional native 3D
`robot_simulator`.

- Select **Run robot_simulator** in VS Code's Run and Debug view to launch
  `build/_deliverables/apps/robot_simulator/Release/robot_simulator.exe`
  with the repository root as its working directory.
- The launch configuration uses the MSVC `cppvsdbg` debugger and runs the
  `build-robot-simulator-release` pre-launch task first. The task builds only
  the `robot_simulator` target in Release configuration.
- The simulator remains optional. Configure CMake with
  `-DBUILD_ROBOT_SIMULATOR=ON` to include it; the option defaults to `OFF`.
  When enabled, the app fetches Raylib 5.5 and Bullet 3.25.
- The VS Code build task is part of the `tasks` array in
  [tasks.json](../../../../.vscode/tasks.json), and the debugger entry is in
  [launch.json](../../../../.vscode/launch.json). The simulator target and
  dependency declarations are in
  [robot_simulator/CMakeLists.txt](../../../../_deliverables/apps/robot_simulator/CMakeLists.txt).

## Verification

- [x] The Release `robot_simulator` target built successfully with
  `cmake --build build --config Release --target robot_simulator`.
- [x] VS Code successfully discovered and ran the
  `build-robot-simulator-release` task.
- [x] The launch configuration parsed with the expected executable,
  `cppvsdbg` debugger, and pre-launch task.
- [x] The simulator's `--diagnostic` run passed motion, lidar, collision,
  localization, exit, and reward checks.
