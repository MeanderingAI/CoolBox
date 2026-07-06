# v1.3.4 addendum: screen_builder app introduction

Status
- Implemented and validated in current workspace.
- Added a dedicated app for XML screen editing and preview.

Summary
- Added new app target: screen_builder.
- Added both GUI-first and CLI editing modes.
- GUI mode is default to avoid stdin-closed behavior in debugger sessions.
- CLI mode remains available via --cli.

Files added
- _deliverables/apps/screen_builder/CMakeLists.txt
- _deliverables/apps/screen_builder/src/main.cpp

Build wiring updated
- _deliverables/apps/CMakeLists.txt

Capabilities
- Load XML file (default path or --xml override).
- Edit externally from GUI (open file in system editor).
- Reload and save from GUI controls.
- CLI operations:
  - view, set, insert, delete, save, reload, preview, quit
- Preview window:
  - parses XML descriptor
  - applies menu bar model to full_application_window
  - shows interactive toolbar actions from descriptor toolbar model

Validation executed
- Build:
  - cmake --build build --config Debug --target screen_builder
- CLI smoke:
  - scripted run verified command loop and clean exit

Acceptance status
- New screen_builder application: complete.
- GUI-first behavior for Run/Debug sessions: complete.
