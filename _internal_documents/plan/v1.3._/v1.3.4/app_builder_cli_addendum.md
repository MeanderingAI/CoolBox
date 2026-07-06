v1.3.4 addendum: app_builder CLI framework and secret_gen_cli app

Status
- Implemented and validated in current workspace.
- New reusable CLI library added under app_builder/OS_GENERICS.
- New secret_gen_cli application added under _deliverables/apps.
- VS Code Run/Debug integration added for build + launch with prompted arguments.

Scope summary
- Added app_builder OS_GENERICS cli_tools library for consistent CLI option parsing.
- Added app executable secret_gen_cli that generates UUIDs by version and batch count.
- Added launch/task wiring so developers can run secret_gen_cli directly from IDE with prompts.

Files added
- _deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/CMakeLists.txt
- _deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/headers/cli_tools.hpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/source/cli_tools.cpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/tests/test_cli_tools.cpp
- _deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/tools/cli_tools_demo.cpp
- _deliverables/apps/secret_gen_cli/CMakeLists.txt
- _deliverables/apps/secret_gen_cli/src/main.cpp

Files updated
- _deliverables/apps/CMakeLists.txt
- .vscode/tasks.json
- .vscode/launch.json

CLI framework details (cli_tools)
- Added OptionSpec, ParseResult, and CommandLineParser API.
- Supports:
  - long options (--version v7, --batch=10)
  - short options (-v v7, -b 10)
  - compact short flags (-abc)
  - required option validation
  - positional argument capture
  - generated help text

secret_gen_cli behavior
- Command options:
  - --version / -v : UUID version selector
  - --batch / -b : number of UUIDs to emit
  - --help / -h : help text
- Supported version values:
  - 1,2,3,4,5,6,7,8,v1..v8,guid
- Output:
  - prints one generated UUID per line to terminal
- Backed by:
  - uuid_generation package (native C++)
  - cli_tools parser library

Build and run integration
- Added app registration in _deliverables/apps/CMakeLists.txt.
- Added VS Code task:
  - build-secret-gen-cli-debug
- Added VS Code launch configurations:
  - Run secret_gen_cli (v7 x 10)
  - Run secret_gen_cli (Prompt Args)
- Added launch input prompts:
  - secretGenUuidVersion
  - secretGenBatchSize

Validation notes
- cli_tools tests built and passed:
  - CliToolsTests: 1/1 passed, 0 failed
- secret_gen_cli built successfully in Debug.
- secret_gen_cli sample run verified:
  - --version v7 --batch 3 produced 3 UUID lines.
- Diagnostics clean on added/updated source and config files.

Release notes bullets for v1.3.4
- Added app_builder OS_GENERICS cli_tools library to standardize command-line parser behavior.
- Added secret_gen_cli application for terminal UUID batch generation by version.
- Added VS Code build/run-debug workflow for secret_gen_cli including prompted runtime options.
