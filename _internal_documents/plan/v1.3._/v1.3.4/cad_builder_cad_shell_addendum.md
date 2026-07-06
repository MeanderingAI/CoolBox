# v1.3.4 addendum: cad_builder CAD shell upgrades

Status
- Implemented and validated in current workspace.
- cad_builder blank mode now behaves like a practical command-driven CAD shell.

Summary
- Upgraded cad_builder from a simple rectangle demo to a typed command workflow with:
  - 3D viewport camera controls (pan/rotate/zoom/reset)
  - multi-geometry support (rectangles, lines, arcs, circles)
  - selection-driven editing and typed direct-edit syntax
  - scene persistence and preview export
  - undo/redo command history snapshots
  - shell command history and replay helpers
  - snap modes with configurable threshold and HUD feedback

Files updated
- _deliverables/apps/cad_builder/src/main.cpp
- _deliverables/apps/cad_builder/src/cad_builder_window.hpp
- _deliverables/apps/cad_builder/src/cad_builder_window.cpp
- _deliverables/apps/cad_builder/CMakeLists.txt

Command shell capabilities
- Geometry creation:
  - add
  - addline
  - addarc
  - addcircle
- Selection:
  - select <index>
  - select <rect|line|arc> <index>
  - ESC to clear selection
- Editing (selection or typed target):
  - move <dx> <dy>
  - move <rect|line|arc> <index> <dx> <dy>
  - resize <a> <b>
  - resize <rect|line|arc> <index> <a> <b>
  - fill [on|off|toggle] (rectangle)
  - duplicate [index]
  - duplicate <rect|line|arc> <index>
  - delete [index]
  - delete <rect|line|arc> <index>
  - clear
- Camera/navigation:
  - pan, yaw, pitch, zoom, resetview, cam
  - aliases: j/l/i/k, u/o/n/m, +/-
- Snap:
  - snap [off|grid|endpoint|center]
  - grid <size>
  - snapthreshold <pixels>
  - snapinfo
- Session/history:
  - history
  - !!
  - !<n>
- Persistence/export:
  - save [path], load [path], export [path]

Scene format notes
- Scene serializer upgraded to HKSCENE2 and now includes:
  - camera state
  - rectangles
  - lines
  - arcs
- Loader remains backward compatible with HKSCENE1.

Viewport notes
- Selection highlighting for rectangle/line/arc.
- HUD now includes selected geometry indices and status text.
- Snap marker overlay is rendered when a point is snapped.

Stability fix
- Added EOF handling in the command loop so piped/scripted runs exit cleanly instead of looping prompt output.

Validation executed
- Build:
  - cmake --build build --config Debug --target cad_builder
- Smoke runs:
  - verified add/list/select/move/resize/fill/duplicate/delete flows
  - verified typed direct-edit forms and typed duplicate/delete forms
  - verified save/load/export and undo/redo behavior

Acceptance status
- CAD-style command shell behavior: complete.
- Multi-geometry editable workflow: complete.
- Typed direct target editing: complete.
- Snap controls and visual feedback: complete.
- Stable scripted execution behavior: complete.
