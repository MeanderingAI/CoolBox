# v1.3.4 addendum: cad_builder OS_GENERICS windowing abstraction pass

Status
- Implemented and validated in current workspace.
- cad_builder interactive GUI path now uses package-level OS abstraction APIs instead of app-local Win32 calls for core GUI behavior.

Summary
- Migrated cad_builder interactive GUI behavior to OS_GENERICS full_application_window APIs for:
  - menu command handling
  - menu check-state updates
  - pointer polling
  - info dialogs
  - primary canvas presentation
  - auxiliary window drawing
- Preserved separate windows for:
  - AI chat dialog
  - Inspector dialog
  - Toolbox selector

Primary files updated
- _deliverables/apps/cad_builder/src/main.cpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/headers/full_application_window.hpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/source/full_application_window.cpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/CMakeLists.txt

API additions used by cad_builder
- set_menu_command_handler(...)
- set_menu_item_checked(...)
- menu_item_checked(...)
- query_pointer_state(...)
- show_info_dialog(...)
- client_size(...)
- clear_background(...)
- fill_rect(...)
- draw_text_line(...)
- present_canvas(...)

Behavior outcomes
- Main viewport rendering now routes through present_canvas.
- Dialog/window painting routes through full_application_window drawing APIs.
- Menu command dispatch is centralized in the window abstraction layer.

Validation executed
- Build:
  - cmake --build build --config Debug --target cad_builder
- Runtime smoke:
  - verified interactive launch and menu commands
  - verified separate auxiliary windows render and close correctly

Acceptance status
- Package-first GUI abstraction migration: complete.
- Dialog separation and package painting path: complete.
