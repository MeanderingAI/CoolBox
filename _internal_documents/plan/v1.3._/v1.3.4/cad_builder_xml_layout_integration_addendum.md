# v1.3.4 addendum: cad_builder XML layout integration

Status
- Implemented and validated in current workspace.
- cad_builder launch path now supports descriptor-driven GUI defaults.

Summary
- Integrated XML descriptor loading into cad_builder interactive startup.
- Added CLI option:
  - --screen-xml <path>
- Added default descriptor file for cad_builder layout.
- Applied descriptor data to:
  - main window title and size
  - platform style
  - menu bar selection when provided in descriptor
  - toolbox labels from toolbar/layout models
  - dialog text/title hints

Files updated
- _deliverables/apps/cad_builder/src/main.cpp
- _deliverables/apps/cad_builder/CMakeLists.txt

Files added
- _deliverables/apps/cad_builder/assets/default_screen_layout.xml

Behavior outcomes
- If --screen-xml is provided, cad_builder loads and applies that descriptor.
- If omitted, cad_builder looks for the default layout file under app assets.
- Menu/toolbar labels can now be controlled from XML model content.

Validation executed
- Build:
  - cmake --build build --config Debug --target cad_builder
- Runtime:
  - verified launch with default descriptor
  - verified launch with explicit --screen-xml path

Acceptance status
- Descriptor-driven cad_builder startup behavior: complete.
