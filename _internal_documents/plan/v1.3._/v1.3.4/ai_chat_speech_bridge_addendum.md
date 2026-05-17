# v1.3.4 addendum: ai_chat speech-to-CAD bridge

Status
- Implemented and validated in current workspace.
- Added new app_assets package ai_chat with speech phrase interpretation and a runnable bridge executable.

Summary
- Added a reusable ai_chat library that converts human speech text into cad_builder commands.
- Added a bridge executable that:
  - accepts transcript lines from stdin or file
  - maps speech to CAD commands
  - emits a runnable command script
  - optionally launches cad_builder with that generated script
- Added a Windows live microphone launcher script for end-to-end voice-driven CAD startup.

Files added
- _deliverables/libraries/groups/app_assets/ai_chat/CMakeLists.txt
- _deliverables/libraries/groups/app_assets/ai_chat/headers/ai_chat.hpp
- _deliverables/libraries/groups/app_assets/ai_chat/source/ai_chat.cpp
- _deliverables/libraries/groups/app_assets/ai_chat/tests/test_ai_chat.cpp
- _deliverables/libraries/groups/app_assets/ai_chat/tools/ai_chat_voice_bridge.cpp
- _deliverables/libraries/groups/app_assets/ai_chat/tools/ai_chat_live_mic.ps1
- _deliverables/libraries/groups/app_assets/ai_chat/tools/README.md

Files updated
- _deliverables/libraries/groups/app_assets/CMakeLists.txt

Targets
- ai_chat_lib (library)
- ai_chat_tests (unit tests)
- ai_chat_voice_bridge (tool executable)

Library behavior (ai_chat_lib)
- Direct command passthrough for explicit CAD shell commands.
- Natural phrase mapping examples:
  - please add line -> addline
  - zoom in 120 -> zoom 120
  - zoom out 80 -> zoom -80
  - pan left 30 -> pan -30 0
  - pan right 20 -> pan 20 0
  - save scene -> save
  - load scene -> load
  - export preview -> export
  - repeat command 3 -> !3
  - repeat last command -> !!
- Queues recognized commands and records interpretation history.

Bridge behavior (ai_chat_voice_bridge)
- Inputs:
  - --transcript-file <path> (optional)
  - stdin transcript lines when no file is provided
- Outputs:
  - --out-script <path> containing cad_builder command script
  - script starts with mode selector 2 and ends with q
- Optional direct launch:
  - --run-cad-builder <path-to-cad_builder.exe>

Live microphone script (Windows)
- ai_chat_live_mic.ps1 uses System.Speech dictation engine.
- Captures spoken lines from default audio device.
- End capture by saying: stop listening
- Passes transcript to ai_chat_voice_bridge and launches cad_builder automatically.

Validation executed
- Build:
  - cmake --build build --config Debug --target ai_chat_lib ai_chat_tests ai_chat_voice_bridge
- Tests:
  - ai_chat_tests: 5 passed, 0 failed
- Bridge smoke:
  - generated command script from transcript input
  - validated mapped script content and cad_builder launch path

Acceptance status
- New app_assets ai_chat package: complete.
- Speech phrase to CAD command mapping: complete.
- Runnable transcript bridge tool: complete.
- Windows live microphone launch script: complete.
