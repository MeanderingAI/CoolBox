# ai_chat Tools

## Live Microphone to CAD Command Bridge (Windows)

Use `ai_chat_live_mic.ps1` to capture speech from the microphone, map phrases to
cad_builder commands, and launch cad_builder automatically.

### Basic usage

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File _deliverables/libraries/groups/app_assets/ai_chat/tools/ai_chat_live_mic.ps1
```

Speak commands such as:
- add line
- add circle
- zoom in 100
- pan left 30
- save scene

Say `stop listening` to end capture and launch cad_builder.

### Use a prepared transcript file (non-mic mode)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File _deliverables/libraries/groups/app_assets/ai_chat/tools/ai_chat_live_mic.ps1 -TranscriptFile build/voice_demo.txt
```

### Optional arguments

- `-BridgeExe <path>`: path to `ai_chat_voice_bridge.exe`
- `-CadBuilderExe <path>`: path to `cad_builder.exe`
- `-OutScript <path>`: generated cad command script path
- `-Quiet`: suppress per-line bridge output
