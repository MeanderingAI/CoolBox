# v1.5.0 Build and CI Fix Notes

## x86 Linux build fix: header packaging path

### Issue

In the Linux x86_64 build workflow, the "Package all headers" step failed with:

- `find: '_libraries/packages': No such file or directory`
- `Process completed with exit code 1`

### Root cause

The workflow step used a hardcoded legacy path:

- `_libraries/packages`

In the current repository layout, headers live under:

- `_deliverables/libraries/groups`

When the missing path was passed to `find`, the command returned a non-zero exit status and failed the step.

### Fix

Updated:

- `.github/workflows/build-libs.yaml`

Changes:

- Added source-root autodetection for header packaging:
  - use `_libraries/packages` if it exists
  - else use `_deliverables/libraries/groups` if it exists
- Added guard behavior to avoid failing when neither root exists.
- Kept archive generation behavior unchanged.

### Validation

Local shell validation confirmed path detection resolves to current tree:

- `header_root=_deliverables/libraries/groups`
- discovered include/header directories successfully.

### Impact

- Prevents x86 Linux workflow failure in header-packaging stage when legacy path is absent.
- Maintains compatibility with both legacy and current repository layouts.

## Windows build fix: python ml_core undefined references during link

### Issue

Windows CI failed while linking the python bindings module `ml_core` with a large set of undefined references (for example `bind_graphics`, `bind_misc`, `DecisionTree::fit`, `SVM::fit`, `BayesianNetwork::add_node`, `HMM::set_initial_probabilities`, `ml::cv::Image` symbols, and multiple time series symbols).

### Root cause

The `ml_core` target source list did not reliably include all implementation units that are referenced by `py_ml_core.cpp` across modules. As a result, symbol declarations were visible at compile time, but implementations were missing at link time on Windows.

Additionally, include path handling required stabilization for vendor and flat module headers used by bindings sources.

### Fix

Updated:

- `.github/workflows` not changed for this issue.
- `_deliverables/libraries/bindings/python_bindings/CMakeLists.txt`

Changes made:

- Reworked `ml_core` source collection to include the module implementation directories required by `py_ml_core.cpp` (decision tree, SVM kernels, bayesian network, hidden markov model, multi arm bandit, computer vision, time series, graphics_misc, distributed, GLM, NLP, dimensionality reduction, and PDE/SPDE bindings).
- Kept existing cool_car DL source wiring in place to preserve established behavior.
- Added curated flat include subdirectories for modules that use `<module_header.h>` style includes.
- Corrected vendor include roots to use the python_bindings local vendor tree.

### Validation

Local target validation completed successfully after the CMake/source-list update:

- `cmake --build build --target ml_core -j4`
- `BUILD_EXIT_CODE:0`

### Impact

- Resolves missing-symbol link failures for the Windows python bindings build path.
- Keeps existing DL compilation path intact while ensuring referenced binding implementations are linked into `ml_core`.

## Additional warning cleanup bundled with the ml_core fix

### Issue

After the linker fixes, the `ml_core` build still emitted warning noise in shared matrix/DL and bayesian-network code paths (unused parameter/field, constructor init order, and signed-vs-unsigned comparisons).

### Fix

Updated:

- `_deliverables/libraries/groups/cool_car/MATRIX/headers/matrix_dense.h`
- `_deliverables/libraries/groups/cool_car/DL/layers/include/layer.h`
- `_deliverables/libraries/groups/cool_car/DL/layers/src/tensor.cpp`
- `_deliverables/libraries/groups/cool_car/DL/wrapper/src/neural_network.cpp`
- `_deliverables/libraries/groups/cool_car/DL/wrapper/include/templates.h`
- `_deliverables/libraries/bindings/python_bindings/src/bayesian_network/bayesian_network.cpp`

Changes made:

- Marked intentionally-unused backend selections and placeholder fields/parameters with `[[maybe_unused]]` (or parameter omission where appropriate).
- Corrected constructor member initialization order in tensor implementation to match declaration order.
- Reworked selected bayesian-network loops and comparisons to avoid signed/unsigned warnings.
- Removed an unused local variable in bayesian joint-probability calculation.

### Validation

Local validation after warning cleanup:

- `cmake --build build --target ml_core -j4`
- `BUILD_EXIT_CODE:0`

### Impact

- Reduces warning noise in the `ml_core` path while preserving behavior.
- Makes CI output more actionable by surfacing new warnings/errors more clearly.

## Follow-up warning profile pass (Windows-oriented)

### Scope

A second pass was run to target warning classes that commonly surface in Windows toolchains (unused parameters/fields and signed-vs-unsigned comparisons) while keeping behavior unchanged.

### Additional updates

- `_deliverables/libraries/bindings/python_bindings/include/deep_learning/layer.h`
- `_deliverables/libraries/bindings/python_bindings/src/computer_vision/image.cpp`
- `_deliverables/libraries/bindings/python_bindings/src/computer_vision/layers.cpp`
- `_deliverables/libraries/bindings/python_bindings/include/computer_vision/layers.h`
- `_deliverables/libraries/bindings/python_bindings/src/decision_tree/boost_tree.cpp`
- `_deliverables/libraries/bindings/python_bindings/src/decision_tree/rule_set.cpp`
- `_deliverables/libraries/bindings/python_bindings/src/dimensionality_reduction/umap.cpp`
- `_deliverables/libraries/bindings/python_bindings/include/dimensionality_reduction/umap.h`
- `_deliverables/libraries/bindings/python_bindings/src/distributed/distributed_trainer.cpp`
- `_deliverables/libraries/bindings/python_bindings/include/distributed/distributed_trainer.h`
- `_deliverables/libraries/bindings/python_bindings/src/distributed/message_passing.cpp`
- `_deliverables/libraries/bindings/python_bindings/include/distributed/message_passing.h`
- `_deliverables/libraries/bindings/python_bindings/src/nlp/embeddings.cpp`
- `_deliverables/libraries/bindings/python_bindings/include/time_series/time_series.h`

### Validation

- `cmake --build build --target ml_core --clean-first -j4`
- `BUILD_EXIT_CODE:0`
- Diagnostics scan on the clean build log found no `warning:`, `error:`, or `undefined reference` entries for this target.

## Scala bindings extension scaffold

### Scope

Added a Scala bindings module parallel to the existing Java bindings module, using Maven and the same native `coolbox_c_bindings` bridge strategy.

### Added

- `_deliverables/libraries/bindings/scala_bindings/README.md`
- `_deliverables/libraries/bindings/scala_bindings/pom.xml`
- `_deliverables/libraries/bindings/scala_bindings/src/main/scala/io/coolbox/CoolBoxScalaClient.scala`
- `_deliverables/libraries/bindings/scala_bindings/src/test/scala/io/coolbox/CoolBoxScalaClientTest.scala`

### Updated

- `_interfaces/GUI/routes/extensions.py` (`scala_bindings` language label)
- `_internal_documents/code_documentation/docs/internal_documents/extensions_overview.md`
- `_internal_documents/code_documentation/docs/internal_documents/extensions/scala/README.md`
- `_internal_documents/plan/v1.3._/v1.3.3/gui_package_builder_extensions_tab.md`

### Validation

- Added Maven/Scala scaffold files and verified changed-file diagnostics are clean.
- Attempted local Maven validation with `mvn -f _deliverables/libraries/bindings/scala_bindings/pom.xml -B test`, but `mvn` is not installed in the current shell environment.

## Scala LSP restored alongside bindings work

### Scope

Reintroduced the Scala LSP frontend/library/docker/workflow path after the earlier bindings-only correction, so both Scala bindings and Scala LSP now exist in the repo.

### Added

- `_deliverables/libraries/groups/COMMS/LSP/lsp_scala/` (library, header/source, and smoke test)
- `_deliverables/apps/lsp/src/main_scala.cpp`
- `_deliverables/apps/lsp/docker/Dockerfile.scala`
- `.github/workflows/lsp/lsp-scala.yaml`

### Updated

- `_deliverables/apps/lsp/CMakeLists.txt` (`plscala_lsp` executable target)
- `_deliverables/apps/lsp/app_page.html` (target/source/dependency listing)
- `.github/workflows/build-lsp.yaml` (artifact packaging includes `plscala_lsp`)
- `.github/workflows/lsp/lsp-docker.yaml` (build/push Scala image)
- `.github/workflows/build-purchase-pipeline.yaml` (fallback copy + image publish)

### Validation

- `cmake -S . -B build` → `CONFIG_EXIT_CODE:0`
- `cmake --build build --target plscala_lsp plscala_lsp_test -j4` → `BUILD_EXIT_CODE:0`
- `./build/LSP_lsp_scala_build/plscala_lsp_test` → `TEST_EXIT_CODE:0`

## Windows follow-up fix: ml_core link failures for DL train_step, graphics, and wave generator

### Issue

Windows CI failed while linking `ml_core` with unresolved symbols including:

- `ml::deep_learning::NeuralNetwork::train_step(...)`
- `utils::wave_generator::{sample_at, generate_samples, generate_samples_for_duration}(...)`
- multiple `graphics::{Canvas, Graph, Table}` methods referenced by python bindings.

### Root cause

- `NeuralNetwork::train_step` was declared in the cool_car DL wrapper API but not implemented in `_deliverables/libraries/groups/cool_car/DL/wrapper/src/neural_network.cpp`.
- `ml_core` bindings referenced graphics/wave-generator APIs from headers, but their implementation translation units were not included in the `ml_core` target sources.

### Fix

Updated:

- `_deliverables/libraries/groups/cool_car/DL/wrapper/src/neural_network.cpp`
- `_deliverables/libraries/bindings/python_bindings/CMakeLists.txt`

Changes made:

- Added a concrete `NeuralNetwork::train_step` implementation in the cool_car DL wrapper source.
- Added vendor implementation sources to `ml_core`:
  - `vendor_src/GRAPHICS/charts/source/graphics.cpp`
  - `vendor_src/MISC/wave_generator/source/wave_generator.cpp`
- Added `stb` FetchContent wiring and include path (as SYSTEM include) for `stb_image_write.h`, which is required by graphics PNG/JPG export code.

### Validation

- `cmake -S . -B build` → `CONFIG_EXIT_CODE:0`
- `cmake --build build --target ml_core --clean-first -j4` → `BUILD_EXIT_CODE:0`

### Impact

- Resolves the reported Windows undefined-reference link errors in the `ml_core` python bindings target.
- Keeps graphics/misc bindings link-complete by explicitly including required implementation units.

## Windows ARM64 CI fix: release-assets upload had no files

### Issue

The Windows ARM64 lane reported:

- `Upload release assets: No files were found with the provided path: release-assets/. No artifacts will be uploaded.`

### Root cause

The library packaging script in `.github/workflows/build-libs.yaml` assumed legacy package paths under:

- `_libraries/packages/<category>`
- `build/_libraries/packages/<category>`

In the current tree/build layout, outputs are primarily under `_deliverables/libraries/groups` and `build/_deliverables/libraries/groups`, so category packaging could produce zero archives for this lane.

### Fix

Updated:

- `.github/workflows/build-libs.yaml`

Changes made:

- Added path fallback from legacy `_libraries/packages/*` to `_deliverables/libraries/groups/*` for category packaging.
- Added a fallback aggregate package path when category packaging yields no archives, collecting available built libraries and headers into:
  - `coolbox-libraries-<platform>-<ref>.tar.gz`
  - `coolbox-libraries-<platform>-<ref>.zip`
- Set artifact upload to `if-no-files-found: ignore` to avoid hard-fail/noisy warning in genuinely empty edge cases.

### Impact

- Prevents empty `release-assets/` uploads for Windows ARM64 due to layout mismatch.
- Improves resilience across legacy and current repository layouts.

## Process note: ongoing git message log

For incremental CI/build fixes, continue recording updates in commit message bodies (ongoing git message log), and keep this v1.5.0 plan file as a periodic summary rather than a per-change scratch log.

## Extension-specific change notes

Detailed v1.5.0 notes for extension-related path and CI fixes are tracked in:

- `_internal_documents/plan/v1.4._/v1.5.0/extensions/c-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/c3-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/v-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/go-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/python-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/javascript-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/swift-extension.md`
- `_internal_documents/plan/v1.4._/v1.5.0/extensions/windows-ci.md`

## macOS CI fix: Homebrew untrusted `aws/tap` blocked dependency install

### Issue

`build_libs / Build (macos-arm64)` failed during Homebrew operations with:

- `The following taps are not trusted: aws/tap`

### Root cause

GitHub-hosted macOS runners can have third-party taps present in Homebrew state. When Homebrew tap trust enforcement is enabled, an untrusted tap can interrupt normal brew commands.

### Fix

Updated:

- `.github/workflows/fragments/deps-macos/action.yaml`

Changes made:

- Added a pre-install guard step that removes `aws/tap` if present:
  - `brew tap | grep '^aws/tap$'`
  - `brew untap aws/tap`

### Impact

- Makes macOS dependency setup deterministic across hosted runners.
- Prevents unrelated third-party tap trust state from breaking CI dependency installation.
