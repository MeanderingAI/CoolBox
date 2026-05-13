# Test Suite Status After Reorganization Fixes

**Date:** May 2026  
**CTest total:** 64 tests  
**Passing:** 16

## Passing Tests

| # | Test | Notes |
|---|------|-------|
| 1 | TystFrameworkTests | |
| 2 | FileBrowserTests | |
| 3 | BowerShellTests | |
| 6 | GraphicsComponentsTests | |
| 7 | ChartsTests | |
| 11 | FullApplicationWindowTests | |
| 12 | InstallerAbstractionTests | |
| 28 | DeepLearningLayerTests | |
| 29 | BayesianNetworkTests | |
| 30 | BayesianNetworkDBTests | Fixed this session |
| 31 | DecisionTreeTests | |
| 36 | HiddenMarkovModelTests | Fixed this session |
| 38 | LatentSentimentAnalysisTests | Fixed this session |
| 39 | MarkedPointProcessTests | Fixed this session |
| 43 | TrackerTests | Fixed this session |
| 45 | FourierTranformsTests | |

## Not Run / Not Built (48 tests)

These targets were not built and are pre-existing failures unrelated to the reorganization. Categories include:

- **Audio** — AudioMixerTests, MusicTheoryTests, NoteSynthesisTests, AudioProcessingTests, AudioSyncTests
- **Video** — VideoDisplayTests, VideoCodecTests, VideoFiltersTests, VideoTimelineTests
- **Motion** — MotionAnalysisTests
- **Graphics** — GraphicsFontsTests, GraphicsCanvasTests
- **Simulation** — WindowsSimulationTests
- **REST API** — RestApiTests (multiple)
- **LSP** (C3, VLang) — multiple parser and LSP test targets
- **Python bindings** — missing `security/fuzzer/fuzzer.h`, `DL/layers/include/tensor.h`
- **C bindings** — test_coolbox_c_bindings
- **Dimensionality reduction** — GaborPatchesTests (built, runtime status TBD)

## Known Pre-existing Issues (Not From Reorganization)

- `std::mutex` copy-deleted error in audio/video targets.
- Python bindings missing headers from unbuilt targets.
- LSP and parser binaries need separate build pipeline.
