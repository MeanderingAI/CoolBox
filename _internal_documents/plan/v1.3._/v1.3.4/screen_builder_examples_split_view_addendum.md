# v1.3.4 addendum: screen_builder examples and split view

Status
- Implemented and validated in current workspace.
- screen_builder now presents examples on left and preview on right.

Summary
- Added screen_builder-owned default layout descriptor.
- Added examples folder with selectable XML examples.
- Implemented in-app dropdown selector for example files.
- Implemented split-pane rendering:
  - left: XML source lines
  - right: parsed preview and toolbar interactions

Files added
- _deliverables/apps/screen_builder/app_layout.xml
- _deliverables/apps/screen_builder/examples/basic_toolbar.xml
- _deliverables/apps/screen_builder/examples/inspector_and_ai.xml
- _deliverables/apps/screen_builder/examples/split_layout.xml

Files updated
- _deliverables/apps/screen_builder/src/main.cpp

Behavior outcomes
- App discovers XML examples from:
  - _deliverables/apps/screen_builder/examples
- Dropdown switches current example and refreshes preview models.
- Preview pane uses descriptor-defined menu/toolbar models through full_application_window context.

Validation executed
- Build:
  - cmake --build build --config Debug --target screen_builder
- Runtime smoke:
  - verified dropdown selection updates XML panel and preview panel content

Acceptance status
- Examples folder integration: complete.
- Left/right split view requirement: complete.
