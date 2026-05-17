# v1.3.4 addendum: VS Code Run and Debug integration for screen_builder

Status
- Implemented and validated in current workspace.
- Added task and launch profiles for direct Run/Debug usage.

Summary
- Added dedicated build task for screen_builder.
- Added launch profiles:
  - Run screen_builder (Default XML)
  - Run screen_builder (Prompt XML)
- Updated defaults to screen_builder-owned layout file.

Files updated
- .vscode/tasks.json
- .vscode/launch.json

Task added
- build-screen-builder-debug

Launch behavior
- Pre-launch build task set to build-screen-builder-debug.
- Default XML argument path:
  - _deliverables/apps/screen_builder/app_layout.xml
- Prompt input default path aligned with screen_builder app layout.

Validation executed
- JSON diagnostics:
  - no syntax/configuration errors reported for tasks.json and launch.json
- Build:
  - task-backed build path compiled screen_builder successfully

Acceptance status
- Run/Debug workflow integration for screen_builder: complete.
