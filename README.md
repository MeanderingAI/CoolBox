# Compiling

## v1.3.5 Sub-Repo Additions

The following repositories are tracked as git submodules for v1.3.5:

1. `GH-7` at `_sub_repos/GH-7` from `https://github.com/MeanderingAI/GH-7`
2. `BH-2` at `_sub_repos/BH-2` from `https://github.com/MeanderingAI/BH-2.git`
3. `bookish-meme` at `_sub_repos/bookish-meme` from `https://github.com/MeanderingAI/bookish-meme`

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/MeanderingAI/CoolBox.git
```

Update submodules after cloning:

```bash
git submodule update --init --recursive
```

## v1.3.5 Bindings Debugging Update

Recent v1.3.5 Python bindings fixes include:

1. [_internal_documents/plan/v1.3._/v1.3.5/python_linear_regression_fit_segfault_fix.md](_internal_documents/plan/v1.3._/v1.3.5/python_linear_regression_fit_segfault_fix.md)
2. [_internal_documents/plan/v1.3._/v1.3.5/python_deep_learning_wrapper_include_fix.md](_internal_documents/plan/v1.3._/v1.3.5/python_deep_learning_wrapper_include_fix.md)
3. [_internal_documents/plan/v1.3._/v1.3.5/python_hmm_bindings_repair.md](_internal_documents/plan/v1.3._/v1.3.5/python_hmm_bindings_repair.md)
4. [_internal_documents/plan/v1.3._/v1.3.5/python_pde_spde_binding_source_inclusion_fix.md](_internal_documents/plan/v1.3._/v1.3.5/python_pde_spde_binding_source_inclusion_fix.md)

Validation status:

1. `python3 -X faulthandler test_bindings.py` passes (`9/9` tests).
2. `ctest -R hmm_python_bindings_test --output-on-failure` passes (`100%`).

Note: for in-place Python extension rebuilds, run from `_deliverables/libraries/bindings/python_bindings`.

## v1.3.5 Build and Test Stabilization Update

Recent v1.3.5 core test and build stabilization fixes include:

1. [_internal_documents/plan/v1.3._/v1.3.5/cp_decomposition_khatri_rao_fix.md](_internal_documents/plan/v1.3._/v1.3.5/cp_decomposition_khatri_rao_fix.md)
2. [_internal_documents/plan/v1.3._/v1.3.5/matrix_profile_tolerance_adjustment.md](_internal_documents/plan/v1.3._/v1.3.5/matrix_profile_tolerance_adjustment.md)
3. [_internal_documents/plan/v1.3._/v1.3.5/gabor_kernel_unnormalized_center_test_fix.md](_internal_documents/plan/v1.3._/v1.3.5/gabor_kernel_unnormalized_center_test_fix.md)
4. [_internal_documents/plan/v1.3._/v1.3.5/radix_sort_msd_stabilization.md](_internal_documents/plan/v1.3._/v1.3.5/radix_sort_msd_stabilization.md)
5. [_internal_documents/plan/v1.3._/v1.3.5/eigen_ctest_pollution_prevention.md](_internal_documents/plan/v1.3._/v1.3.5/eigen_ctest_pollution_prevention.md)
6. [_internal_documents/plan/v1.3._/v1.3.5/macos_run_debug_terminal_profiles.md](_internal_documents/plan/v1.3._/v1.3.5/macos_run_debug_terminal_profiles.md)

Validation status:

1. `ctest --output-on-failure -j 8` passes (`100%`, `36/36`).
2. `make test` passes with the same clean CTest result (`100%`, `36/36`).

## Release Assets: Platform and Binding Labels

Starting with v1.1.58, all release assets for purchase workflows (including `manifest.txt` and `SHA256SUMS`) are uploaded with unique names that include both the platform and binding type. This prevents asset name collisions and makes it clear which asset corresponds to which package and platform.

**Example asset names:**

- `coolbox-python-bindings-linux-x86_64-v1.1.58.manifest.txt`
- `coolbox-r-bindings-ubuntu-x86_64-v1.1.58.SHA256SUMS`
- `coolbox-c3-bindings-windows-x86_64-v1.1.58.manifest.txt`

This applies to all supported language bindings (Python, R, Go, C, Java, JS, Rust, C3, V, etc.) and all platforms (Linux, macOS, Windows, etc.).

**Why?**

Previously, generic asset names caused GitHub Actions release upload failures due to name collisions. Now, each asset is clearly labeled for its binding and platform, ensuring reliable uploads and easier identification.

# Tool Box

The cmake generates shared objects which can be used with other projects for each of these categories of machine learning.
We also include gunit tests for the entire suite.

## C++ GIS Library

The [GIS library](_deliverables/libraries/groups/trekker/GIS/headers/gis.hpp)
provides WGS84 longitude/latitude to Web Mercator (EPSG:3857) conversion and its
inverse, spherical haversine distances in metres, planar geometry operations,
bounding boxes, spatial candidate queries, and GeoJSON reading/writing.

After `make install`, use `#include <CB/gis.hpp>` and link `CoolBox::gis`:

```cmake
find_package(CoolBox REQUIRED CONFIG)
target_link_libraries(my_app PRIVATE CoolBox::gis)
```

Set `CMAKE_PREFIX_PATH` to your `INSTALL_PREFIX` when configuring the consuming
project. The existing CoolBox JSON library is a transitive dependency.

```cpp
#include <CB/gis.hpp>

auto document = trekker::gis::read_geojson(
    R"({"type":"Point","coordinates":[-73.9857,40.7484]})");
auto projected = trekker::gis::to_web_mercator({-73.9857, 40.7484});
auto distance = trekker::gis::haversine_distance({0, 0}, {1, 0});
auto json = trekker::gis::write_geojson(document);
```

- All seven GeoJSON geometry types, Features, and FeatureCollections are
  supported. Positions use `[longitude, latitude]` with optional altitude;
  properties, string/numeric IDs, bounding boxes, and foreign members survive
  round trips. Null feature geometry/properties and empty multi-geometries or
  collections are supported. Empty Points, LineStrings, and Polygons are rejected.
- Invalid values throw exceptions. GeoJSON accepts finite WGS84 positions
  within longitude `[-180,180]` and latitude `[-90,90]`; legacy `crs` overrides
  and positions with more than three ordinates are rejected.
- Mercator conversion rejects latitudes beyond approximately +/-85.051129
  degrees rather than silently clamping. Haversine distances use a mean-radius
  sphere, not ellipsoidal geodesics.
- `planar_length`, `planar_area`, `covers`, and `bounds` operate in the input
  coordinate plane and ignore altitude. Project first for metre-based lengths
  and square-metre areas, noting that Mercator distorts both. Polygon rings
  must be closed; holes are subtracted regardless of winding. `covers` includes
  exterior and hole boundaries, but excludes hole interiors. These operations
  assume simple polygons with nonoverlapping holes inside the exterior; they
  do not perform topology repair or validation.
- `BBTree` is a balanced binary bounding-box hierarchy. `SpatialIndex` uses it
  internally while preserving its existing API. Both support insertion,
  replacement by ID, removal, and boundary-inclusive point/box queries with
  results sorted by ID. Queries prune subtrees whose bounding boxes do not
  overlap. The tree is rebuilt lazily after edits using median splits along
  the widest axis, allowing batched updates. Rebuilding costs O(n log n);
  queries can still visit O(n) nodes for heavily overlapping boxes, plus
  O(k log k) to sort k matches. `BBTree::height()` reports the balanced tree
  height (zero when empty) and also triggers any pending rebuild.
  Queries update the internal cache, so concurrent access requires external
  synchronization. This is not an R-tree or an exact geometry-intersection
  engine. Planar bounding boxes do not wrap across the antimeridian.

```cpp
trekker::gis::BBTree tree;
tree.insert(42, {0, 0, 10, 10});
auto matches = tree.query(trekker::gis::Coordinate{5, 5, {}});
tree.insert(42, {20, 20, 30, 30}); // Replace the box for this ID.
tree.erase(42);
```

Run the focused tests with `ctest --test-dir build -R "^GisTests$" --output-on-failure`
after building `gis_tests` with `BUILD_TESTING=ON`.

## C++ BBTree Ordered Map

[BBTreeMap](_deliverables/libraries/groups/trekker/DATASTRUCTURE/trees/bbtree/headers/bbtree.hpp)
is a separate, header-only ordered map in `data_structures`, inspired by
[Xiangpeng Hao's bbtree article](https://bbtree.xiangpeng.systems/post/).
It is not the GIS bounding-box `BBTree`, a port of the Rust crate, or a
drop-in replacement for `std::map`.

```cmake
find_package(CoolBox REQUIRED CONFIG)
target_link_libraries(my_app PRIVATE CoolBox::bbtree)
```

```cpp
#include <CB/bbtree.hpp>

data_structures::BBTreeMap<std::string, int> map;
map.insert("apple", 1);                 // Leaves existing keys unchanged.
map.insert_or_assign("banana", 2);     // Inserts or replaces a value.
map.at("apple") = 3;
for (auto it = map.lower_bound("a"); it != map.lower_bound("c"); ++it) {
    // Entries in the half-open range ["a", "c").
}
map.erase("banana");
```

- Balanced B+ nodes hold up to 15 entries by default (minimum degree 8).
  Insertion splits full nodes; deletion borrows or merges underfull nodes
  and collapses the root. Linked leaves support forward ordered iteration.
- String keys with the standard ascending comparator use per-node common-prefix
  lengths and dense 64-bit inline hints; ties compare the full key, including
  embedded null bytes. Unsigned integer keys up to 64 bits also use inline
  hints. Other key/comparator combinations use ordinary binary search.
- Supports `insert`, `insert_or_assign`, `find`, `contains`, `at`, `erase`,
  `lower_bound`, `upper_bound`, mutable/const forward iteration, `clear`,
  copy/move, and `swap`. Keys are immutable; mapped values may be move-only
  when the map is not copied. `validate()` checks structural invariants.
- All insertions, replacements, erasures, and clearing invalidate iterators
  and references conservatively. Use external synchronization for shared access.
  Keys must be copy-constructible, and key comparators must define a strict weak
  order. This API uses forward rather than bidirectional iterators and does not
  implement `std::map`'s allocator, node-handle, or heterogeneous-lookup APIs.
- Nodes own separate entry allocations through shared pointers so internal
  separators never duplicate mapped values. This favors safe generic C++
  ownership over matching the Rust crate's memory layout. No SIMD, concurrent
  variant, async batch mode, or article-level speed/memory claims are provided.
  Search/update are logarithmic for fixed node degree; scanning k entries
  after a bound lookup is O(k).

Build `bbtree_tests` with `BUILD_TESTING=ON`, then run
`ctest --test-dir build -R "^BBTreeMapTests$" --output-on-failure`.

## Native C++ Terminal Widgets

The existing `CoolBox::cli_tools` library now includes
[terminal UI tools](_deliverables/libraries/groups/app_builder/OS_GENERICS/cli_tools/headers/terminal_ui.hpp)
in `os_generics::cli::tui`, alongside its command-line argument parser.
No new third-party dependencies are required.

```cmake
find_package(CoolBox REQUIRED CONFIG)
target_link_libraries(my_app PRIVATE CoolBox::cli_tools)
```

```cpp
#include <CB/cli_tools.hpp>

namespace tui = os_generics::cli::tui;

tui::DocumentEditor editor("Start writing here.\n");
tui::TerminalSession terminal;
for (;;) {
    const auto size = terminal.size();
    tui::Frame frame(size.columns, size.rows);
    editor.render(frame);
    terminal.present(frame);
    const auto event = terminal.read_key();
    if (!event) continue;
    if (event->control && event->character == 'q') break;
    if (event->control && event->character == 's') editor.save("document.txt");
    else editor.handle(*event);
}
terminal.close();
```

Replace the editor with a table in the same event/render loop:

```cpp
tui::TableViewer table({"Name", "Value"}, {{"alpha", "10"}, {"beta", "20"}});
table.set_selection_mode(tui::SelectionMode::cells); // Or SelectionMode::rows.
// table.handle(event); table.render(frame);
// table.active_cell(), table.selected(row, column), table.selection()
```

- `DocumentEditor`: multiline insertion, splitting/joining lines, Backspace/Delete,
  arrows, Home/End, Page Up/Down, horizontal/vertical scrolling, cursor highlighting,
  a dirty flag, and explicit `load(path)` / `save(path)` operations. Tab expands
  to four spaces; loading normalizes CRLF to LF and expands tabs. Saving uses LF
  and overwrites the destination directly (not an atomic or backup save).
  File and validation failures throw; failed loads preserve the current document.
- `TableViewer`: arrow/Home/End/Page navigation, automatic scrolling, Space to
  toggle a cell or row, Shift+navigation to select a rectangular cell range or
  contiguous row range, Escape to clear selections. Switching selection modes
  clears the selection. Row selections are returned once per row with column zero.
  The table is read-only and requires rectangular data with at least one column;
  zero rows are supported. Column widths are inferred and capped at 30 characters.
- `Menu`: Up/Down/Home/End navigation, scrolling, Enter activation.
  `Frame`: clipped styled text drawing, plain-text snapshots, ANSI rendering,
  and a status bar. Widget rendering clears its frame; widgets are full-frame,
  not a multi-pane layout manager.
- `TerminalSession`: Windows console events and ANSI output; POSIX termios/poll
  input on Linux/macOS; alternate screen, hidden hardware cursor, raw keys,
  terminal size checks, configurable input timeouts, and mode restoration.
  It requires an actual interactive console and rejects redirected streams.
  Destruction reports cleanup errors to stderr; `close()` permits callers to
  catch them explicitly. Handle application exceptions outside the session's
  scope so unwinding restores the terminal.
- Initial text support is printable ASCII, not UTF-8/grapheme-aware. Unsupported
  input produces `Key::unknown`; invalid widget strings (including terminal
  escape characters) throw instead of being emitted as control sequences.
  No mouse, clipboard, undo/redo, text selection, CSV parser, or syntax
  highlighting is included. Ctrl/Alt shortcuts are left to the host application.
  POSIX Shift+navigation requires xterm-style modified key sequences.
- Use one session per terminal. Call `size()` again when rendering after a resize;
  avoid continuously repainting when no input or resize occurred in production.
  Ctrl+C is delivered as a key rather than terminating the application; fatal
  process termination cannot guarantee terminal restoration.

Build `cli_tools_tests` with `BUILD_TESTING=ON`, then run
`ctest --test-dir build -R "^CliToolsTests$" --output-on-failure`.

## Python Install

The Python bindings can now be installed directly from the repository root:

```bash
pip install git+https://github.com/MeanderingAI/CoolBox.git
```

Local source installs also work from the repository root:

```bash
pip install .
```

The Python package exposes both `ml_toolbox` and the legacy compatibility import `ml_core`.

## Filters

Here we provide a Kalman Filter library which contains CPP implementation of the standard array of implementations;

1. Standard Kalman Filter
2. Extended Kalman Filter
3. Unscented Kalman Filter
4. Sequential Monte Carlo


## Generalized Linear Models

Current implementation includes Linear Regression with;

1. Stochastic Gradient Decent
2. Closed Form Solution

## Decision Trees

Here we provide the following Decision Tree Estimators;

1. Entropy 
2. Gini Index

With the following variations;

1. Rule Set
2. Random Forest
3. Boosted Trees

## Statistical Distributions

Here we provide the following Standard Statistical Distributions;

1. Bernoulli Distribution
2. Binomial Distribution
3. Categorical Distribution
4. Exponential Distribution
5. Gamma Distribution
6. Inverse Gaussian Distribution
7. Laplace Distribution
8. Multinomial Distribution
9. Normal Distribution
10. Poisson Distribution

These distributions have implementations of pdf, log_pdf, cdf, log_cdf, and sampling.

## Hidden Markov Model

Traditional implementation of forward and backward algorithm.

## Multi-Arm Bandit

Here we provide the following multi-arm bandit implementations;

1. Epsilon Greedy Agent
2. Decaying Epsilon Greedy Agent
3. Upper Confidence Bound Agent 
4. Thompson Sampling Agent

## Support Vector Machine

Here we provide implementation of support vector machines with the following kernels;

1. Linear
2. Polynomial
3. Gaussian 
4. Sigmoid

## Latent Sentiment Analysis

Perform latent sentiment analysis on text using matrix factorization:

1. Document-term matrix factorization
2. Latent feature extraction for documents and terms
3. SGD optimization with regularization
4. Sentiment prediction and reconstruction

## Hidden Markov Models

Provides comprehensive HMM implementation:

1. Forward-Backward algorithm (evaluation)
2. Viterbi algorithm (decoding)
3. Baum-Welch algorithm (training)
4. Log-likelihood computation
5. Multiple observation sequences support

## Bayesian Networks

Directed Acyclic Graph (DAG) probabilistic models:

1. Node and edge management
2. Conditional Probability Tables (CPT)
3. Joint probability calculation
4. Probabilistic inference
5. Evidence-based reasoning

## Marked Point Process

Temporal event modeling with associated marks (labels):

1. Self-exciting Hawkes processes
2. Multi-mark event sequences
3. Conditional intensity prediction
4. Event sequence generation
5. Maximum likelihood estimation
6. Applications: financial transactions, user activity logs, earthquakes

## Piecewise Conditional Intensity Models (PCIM)

Non-stationary temporal point process modeling:

1. Multiple intensity function types:
   - Constant intensity
   - Linear intensity
   - Exponential decay
   - Hawkes self-exciting
   - Cox proportional hazards
2. Uniform and adaptive interval creation
3. Regime change detection
4. Model selection (AIC/BIC)
5. Applications: market microstructure, crime patterns, healthcare monitoring

## Dimensionality Reduction

Comprehensive suite of dimensionality reduction algorithms for feature extraction, visualization, and manifold learning:

### 1. **Singular Value Decomposition (SVD)**
   - Thin and full SVD computation using Eigen's BDCSVD
   - Low-rank matrix approximation and reconstruction
   - Matrix rank estimation with tolerance
   - Explained variance analysis
   - Applications: matrix compression, noise reduction, latent semantic analysis

### 2. **Principal Component Analysis (PCA)**
   - Linear dimensionality reduction via SVD
   - Data centering and optional standardization (scaling)
   - Principal component extraction (loadings)
   - Variance explained computation
   - Forward and inverse transformation
   - Applications: data visualization, feature extraction, preprocessing

### 3. **k-Nearest Neighbors (KNN)**
   - Efficient brute-force nearest neighbor search
   - Multiple distance metrics: Euclidean, Manhattan, Cosine
   - Pairwise distance computation
   - Self-exclusion option for training data queries
   - Foundation for manifold learning algorithms

### 4. **Uniform Manifold Approximation and Projection (UMAP)**
   - State-of-the-art non-linear dimensionality reduction
   - Preserves both local and global data structure
   - Fuzzy simplicial set construction
   - Stochastic gradient descent optimization
   - Superior to t-SNE for many applications
   - Applications: visualization, cluster analysis, anomaly detection

**Example (C++ with mytrix):**

```cpp
#include "dimensionality_reduction/pca.h"
#include "dimensionality_reduction/knn.h"
#include "dimensionality_reduction/umap.h"

// PCA for linear dimensionality reduction
mytrix::Matrix X = /* your data (n_samples x n_features) */;
PCA pca(10, true, false);  // 10 components, center=true, scale=false
pca.fit(X);
mytrix::Matrix X_pca = pca.transform(X);

// KNN for nearest neighbor queries
KNN knn(15, "euclidean");  // k=15 neighbors
knn.fit(X);
auto [indices, distances] = knn.kneighbors();

// UMAP for non-linear manifold learning
UMAP umap(2, 15, 0.1, "euclidean", 1.0, 200, 42);
// n_components=2, n_neighbors=15, min_dist=0.1
mytrix::Matrix X_umap = umap.fit_transform(X);
```

**Key Differences:**
- **PCA**: Linear, fast, preserves global variance structure
- **UMAP**: Non-linear, slower, preserves local neighborhood structure and global topology
- **t-SNE vs UMAP**: UMAP scales better, faster convergence, preserves more global structure

**Notes:**
- Use PCA when relationships are linear or for quick exploration
- Use UMAP for complex manifolds and better cluster separation
- For very high-dimensional data (>50 features), consider PCA preprocessing before UMAP
- All algorithms exposed via Python bindings in `ml_core.dimensionality_reduction`
- Scaling recommended when features have different units/scales

## Signal Processing (SP)

The `cool_car/SP` group provides a suite of digital signal processing libraries in the `sp` namespace.

### Filters (`sp::filters`)

FIR and IIR digital filter design and application:

1. **FIR design** — windowed-sinc lowpass, highpass, bandpass, bandstop (Hann window)
2. **IIR design** — Butterworth (lowpass, highpass, bandpass) and Chebyshev Type I (lowpass, highpass) via bilinear transform; biquad notch
3. **Application** — `apply_fir`, `apply_iir` (Direct Form II Transposed), `filtfilt` (zero-phase forward-backward)
4. **Analysis** — `frequency_response`, `group_delay`, `impulse_response`
5. **Smoothing** — `moving_average`, `median_filter`, `savitzky_golay`

### Resampling (`sp::resampling`)

Sample-rate conversion at integer and fractional ratios:

1. **Integer-ratio** — `upsample`, `downsample`, `decimate` (with anti-alias FIR)
2. **Fractional** — `resample_rational` (polyphase windowed-sinc)
3. **Interpolation** — `resample_linear`, `resample_cubic` (Catmull-Rom), `resample_sinc`
4. **Arbitrary rate** — `resample_rate` (floating-point ratio)
5. **Polyphase** — `polyphase_decompose`

### Wavelets (`sp::wavelets`)

Discrete and continuous wavelet transforms:

1. **Families** — Haar, Daubechies 2 & 4, Symlet 2, Coiflet 1, Biorthogonal Spline 1.3
2. **DWT** — `dwt` / `idwt` (single level, periodic extension), `wavedec` / `waverec` (multi-level)
3. **CWT** — `cwt_morlet` (Morlet wavelet, configurable scales), `scalogram`
4. **Denoising** — `threshold` (hard/soft), `denoise` (VisuShrink via MAD noise estimate)
5. **Utilities** — `wavelet_filter_lo/hi`, `cwt_scales`, `pad_to_power_of_two`

## Data Mining (DM)

The `cool_car/DM` group provides time-series data mining algorithms in the `dm` namespace.

### Matrix Profile (`dm::matrix_profile`)

Efficient time-series motif and discord discovery via the matrix profile:

1. **Core computation**
   - `self_join` — STOMP O(n²) matrix profile of a series against itself with O(1) QT diagonal update per pair
   - `ab_join` — cross matrix profile between two independent time series
   - `distance_profile` — z-normalised Euclidean distances from a single query to all windows

2. **Preprocessing**
   - `z_normalize` — zero-mean, unit-variance normalisation of a subsequence
   - `sliding_statistics` — O(n) running mean and standard deviation for all length-m windows

3. **Pattern discovery**
   - `top_motifs` — top-k repeating patterns (lowest distance pairs) with exclusion-zone suppression
   - `top_discords` — top-k anomalies (highest distances) with exclusion-zone suppression

4. **Segmentation (FLOSS)**
   - `arc_curve` — corrected arc curve (CAC) for regime-change detection
   - `segmentation_points` — boundary indices from local minima of the arc curve

5. **Utilities**
   - `extract_subsequence` — slice a subsequence from a time series
   - `batch_self_join` — self-join for every row of a `mytrix::DenseMatrix`

**Example (C++):**

```cpp
#include "matrix_profile.hpp"

std::vector<double> ts = /* your time series */;
std::size_t m = 16;  // subsequence length

// Compute matrix profile
auto mp = dm::matrix_profile::self_join(ts, m);

// Find top motifs (repeating patterns)
auto motifs = dm::matrix_profile::top_motifs(mp, 3);

// Find top discords (anomalies)
auto discords = dm::matrix_profile::top_discords(mp, 2);

// Segment the time series
auto arc = dm::matrix_profile::arc_curve(mp);
auto boundaries = dm::matrix_profile::segmentation_points(arc, 3);
```

# Dependencies

## CMAKE

OSX
```
brew install cmake
```

## GSL

This library uses libgsl for the distribution generation, on ubuntu it can be installed with

Ubuntu
```
sudo apt install libgsl-dev
```

OSX
```
brew install gsl
```

## Bison

```
brew install bison
```

## Doxygen

```
brew install doxygen
```

## Eigen

This library is automatically installed by cmake during compilation

## Test framework

The repository uses its local `tyst_framework` package for C++ tests

# Compiling


## Dependencies

- C++17 compiler (clang++/g++)
- CMake 3.12+
- Python 3 (for bindings)
- **Rust (cargo)**: Required for HTTP/3/QUIC support (quiche library)
   - Install on macOS: `brew install rust`
   - Or: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

Use the `./clean&build` script to build everything.

Generated libraries can be found in;

`build/libraries/src/lib*.so`

# Python Bindings

Python bindings are available for all machine learning algorithms via pybind11. See the `python_bindings/` directory for detailed documentation and examples.

# R Bindings

An initial R package scaffold now lives in `_libraries/r_bindings/coolboxr`. It uses `Rcpp` to expose a native CoolBox linear regression wrapper, `roxygen2` for API docs, and `pkgdown` for a static documentation site.

## Unified Documentation Site

The documentation publishing workflow is defined in `.github/workflows/docs-publish.yaml`. It builds a unified static site with:

- C++ API docs from Doxygen
- R extension docs from pkgdown
- Python bindings docs rendered from the generated Markdown reference
- JavaScript / Emscripten bindings index
- built Python binding artifacts
- built JavaScript / WASM binding artifacts

The generated site includes a top-level `index.html` that links to each documentation section.

Set the repository variable `DOCS_PUBLISH_BRANCH` to choose the target branch for normal pushes, or override it manually with the `publish_branch` input when running the workflow by hand.

Useful entry points:

- `make build_docs_portal` builds the unified static docs site into `.site/`.
- `make document_r_bindings` regenerates roxygen docs.
- `make install_r_bindings` installs the package locally.
- `make site_r_bindings` builds the pkgdown site into `_libraries/r_bindings/coolboxr/docs`.

## Quick Start

```bash
cd python_bindings
./build.sh
python3 test_bindings.py
```

## Available Modules

- `ml_core.decision_tree` - Decision tree algorithms
- `ml_core.svm` - Support Vector Machines with various kernels
- `ml_core.bayesian_network` - Bayesian Network inference
- `ml_core.hmm` - Hidden Markov Models
- `ml_core.glm` - Generalized Linear Models
- `ml_core.multi_arm_bandit` - Multi-arm bandit algorithms
- `ml_core.marked_point_process` - Marked point processes and PCIM
- `ml_core.latent_sentiment_analysis` - Latent sentiment analysis
- `ml_core.tracker` - Kalman filters

See `python_bindings/README.md` and `python_bindings/examples/` for comprehensive usage examples.

## CMake Package (External Projects)

Install the standalone C++ package without configuring unrelated apps, documentation
tools, or language bindings:

```bash
make install
# Optional custom destination (quote paths containing spaces):
make install INSTALL_PREFIX="<install-prefix>"
```

This builds UUID generation, JSON, GIS (including its spatial BBTree), the
ordered-map BBTree, CLI widgets, and deep-learning scaling utilities. It uses a
separate `build-install` directory, leaving existing `build` configurations
untouched. Override `INSTALL_BUILD_DIR` to change that directory. The default
destination is `%LOCALAPPDATA%\CoolBox` on Windows and `~/.local` on Linux/macOS.
Only a C++17 compiler and CMake are required; OpenSSL is optional for UUID
generation, as in the full build.
The Windows Makefile selects Visual Studio x64 by default; set
`VS_GENERATOR_PLATFORM=ARM64` to use an installed ARM64 toolchain instead,
or set `VS_GENERATOR` to choose another CMake generator.

The equivalent direct CMake commands are:

```bash
cmake -S . -B build-install -DCOOLBOX_STANDALONE_INSTALL=ON -DCOOLBOX_ENABLE_GPU_ACCELERATION=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-install --config Release
cmake --install build-install --config Release --prefix <install-prefix>
```

Headers are installed under `include/CB`, libraries under `lib` (or the platform's
GNUInstallDirs library directory), and Windows DLLs under `bin`. Add the installed
`bin` directory to `PATH` when running Windows consumers, or deploy the DLLs next
to the consuming executable. For example:

```cpp
#include <CB/uuid_generation.hpp>
#include <CB/gis.hpp>
#include <CB/bbtree.hpp>
#include <CB/cli_tools.hpp>
#include <CB/dl_scaling/zero_quant.h>
```

To build and install the full repository instead (requires its external
dependencies, including binding/tool dependencies):

```bash
cmake -S . -B build -DCOOLBOX_INSTALL_PACKAGE=ON
cmake --build build --config Debug
cmake --install build --config Debug --prefix <install-prefix>
```

Consume from another CMake project:

```cmake
find_package(CoolBox CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE
    CoolBox::uuid_generation CoolBox::gis CoolBox::bbtree CoolBox::cli_tools
    CoolBox::dl_scaling)
```

If CoolBox is installed to a custom location, set `CMAKE_PREFIX_PATH` to `<install-prefix>`.

To verify the installed package with a separate consumer:

```bash
cmake -S cmake/tests/install_consumer -B build-consumer -DCMAKE_PREFIX_PATH="<install-prefix>"
cmake --build build-consumer --config Release
ctest --test-dir build-consumer -C Release --output-on-failure
```

Use the same compiler and architecture as the installed libraries (add `-A x64`
when configuring the consumer with Visual Studio if needed).

## Internal Maintainer Notes

Internal extension documentation is available in `docs/internal_documents/`, with the main index at `docs/internal_documents/README.md`.
