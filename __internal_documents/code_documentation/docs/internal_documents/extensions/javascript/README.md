# JavaScript And Emscripten Extension

## Location

- `_libraries/emscripten_bindings`

## Surface

- Documentation namespace: `coolbox::javascript`
- Emscripten-generated JavaScript modules and associated WebAssembly outputs
- Exposes selected CoolBox components for browser or JavaScript runtime use
- Generated entry points remain per-target factory exports such as `createAdvancedLoggingModule`

## Included Targets

- `advanced_logging`
- `battery`

- `data_structures`
- `gabor_patches`
- `decision_tree`
- `bayesian_network`
- `hidden_markov_model`
- `distribution`
- `glm`
- `multi_arm_bandit`
- additional ML-oriented targets defined in `_libraries/emscripten_bindings/CMakeLists.txt`

## Installation

This extension is produced from source rather than installed as a local package. Set up Emscripten and build the generated JavaScript outputs.

## Setup And Build

Prerequisites:

- Emscripten SDK
- CMake
- Ninja

Typical local setup:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

Then build from the CoolBox repository root:

```bash
emcmake cmake -S _libraries/emscripten_bindings -B build-emscripten -G Ninja -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN_MODULARIZE=ON
cmake --build build-emscripten --config Release
```

## Setup Notes

- The release workflow sets `COOLBOX_LIB_DIR` to the downloaded native build tree before configuring Emscripten.
- `EMSCRIPTEN_MODULARIZE=ON` is the workflow default, so generated modules export factory functions such as `createAdvancedLoggingModule` and `createDecisionTreeModule`.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `javascript-extension-*` artifacts.