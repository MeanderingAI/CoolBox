# CoolBox Go Bindings

Go bindings for the CoolBox APIs shared by the Python package. The bindings use
cgo to reuse native C/C++ implementations and provide Go implementations for
K-Means and KNN. Python is not required at runtime.

The Go package currently covers the metadata `Client`, UUID generation,
`glm.LinearRegression`, K-Means clustering, KNN search, categorical decision
trees, decision-tree and boosted-tree regression/classification, the Bernoulli
`BanditArm`, native text processing, and univariate time-series statistics,
transforms, windows, autocorrelation, and moving-average, exponential-smoothing,
AR, ARMA, and ARIMA forecasters.

The DL APIs include tensors and their arithmetic, dense/activation/convolutional/
pooling/recurrent layers, losses, optimizers, sequential neural networks, and
the HELiX finite-hypothesis learner. Graphics bindings include native line,
bar, and scatter plots, single-series 2D/3D plotting helpers with X/Y/Z defaults,
RGBA access, and BMP, PNG, and JPG export. `Plot2DOptions.ShowWindow` and
`Plot3DOptions.ShowWindow` open an interactive browser window: scroll to zoom,
drag to pan, and use Save PNG (or S) to export the original plot.

The native data-structure bindings currently cover int64 singly and doubly
linked lists (including the concurrent list), FIFO folio queues, both binary
heap implementations, indexed priority queues, regular and concurrent int64
hash maps, binary-search/red-black/splay trees, order-4 B+ and Maple trees, a
string-keyed radix tree, int64 log-structured merge trees, standard and
faster float64 cover trees, Bloom and EGH filters, Van Emde Boas trees, and
one- through four-dimensional float64 xarrays.

The `DenseMatrix` and `DenseVector` APIs expose the CoolBox MATRIX package's
row-major dense operations, including matrix arithmetic, matrix products,
transpose, norms, vector dot/add, row-wise vector broadcasting, and selectable
CPU optimizations.

## OPT, MATH, and FINANCE

The Go API exposes genetic search, simulated annealing, hill climbing, and
quantum-control Bayesian optimization through `NewSearchOptimizer`.
Pass a corresponding config from `DefaultGeneticSearchConfig`,
`DefaultSimulatedAnnealingConfig`, `DefaultHillClimbingConfig`, or
`DefaultQuantumControlConfig`, then provide an objective callback to
`Optimize`. Quantum-control bounds and frequency warp/unwarp helpers are
available on the optimizer. The distributed-learning bindings include naive
parameter averaging and optimal weighted averaging for linear-regression
models, fast leave-one-model-out cross-validation, and nonlinear optimal
weighted averaging over deep-learning tensor layers. Configure OWA methods
with `DefaultOwaConfig`.

`SolveHeatEquation` handles the package's explicit 1D, 2D, and 3D heat solvers;
set `stochastic` to use the matching seeded SPDE solver. `HeatGrid.Values` are
row-major, with 3D values laid out as `[x][y][z]`.

Finance functions are available through `EvaluateFinanceFormula` and the
`FinanceFormula` constants (including Black-Scholes, digital, Asian, and
barrier options), plus Brownian/GBM paths, Ito integral/variance, and Monte
Carlo estimates/Greeks for built-in European, Asian, and barrier contracts.
Custom-payoff estimates and finite-difference Greeks are also available.
Custom payoff callbacks receive the full simulated price path, including the
initial spot, run synchronously, and may return an error to abort calculation.
For example:

```go
price, err := coolbox.EvaluateFinanceFormula(
	coolbox.BlackScholesCallPrice, 100, 100, 0.05, 0.2, 1,
)
```

`FastOwaCrossValidationLinearRegression` takes one validation fold per local
model; each feature row must match the model parameter width.
`NonlinearOptimalWeightedAverageLayers` accepts models as `[][]*Tensor`, and
its callback receives copied `TensorData` values for candidate merged layers.
The returned `NowaResult.MergedLayers` tensors are caller-owned and should be
closed when no longer needed.

This is not yet full Python API parity. Advanced deep-learning layers (such as
transformers and attention), SVMs, Bayesian networks, HMMs, PCA/SVD/UMAP,
computer vision, distributed training, trackers, advanced 3D chart types,
function/parametric/polar plots, histograms, tables, other data structures
(including B-trees, B+ trees with other key/value types and orders, and cover
trees with custom distance functions), and the remaining NLP and time-series
APIs still need Go bindings.

## Prerequisites

- Go 1.21 or newer with cgo enabled
- A C and C++ compiler compatible with the Go toolchain
- CMake
- On Windows, a MinGW-w64 toolchain compatible with cgo

## Build the native C API

Build the shared C API library before compiling the Go package. On Linux and
macOS, use the default CMake generator and the same C/C++ toolchain as cgo:

```sh
cmake -S _deliverables/libraries/bindings/c_bindings \
  -B _deliverables/libraries/bindings/c_bindings/build \
  -DBUILD_TESTING=ON
cmake --build _deliverables/libraries/bindings/c_bindings/build --config Release
```

On Windows, use a MinGW-w64 toolchain compatible with cgo:

```powershell
cmake -G "MinGW Makefiles" -S _deliverables/libraries/bindings/c_bindings `
  -B _deliverables/libraries/bindings/c_bindings/build `
  -DBUILD_TESTING=ON
cmake --build _deliverables/libraries/bindings/c_bindings/build
```

## Test the Go package

Run the Go tests from the binding directory:

```powershell
Set-Location _deliverables/libraries/bindings/go_bindings
go test ./...
```

If the C API library is built elsewhere, add its directory to `CGO_LDFLAGS`
before running Go commands. On Windows, ensure `coolbox_c_bindings.dll` and its
`metadata_management.dll` and `uuid_generation.dll` dependencies are on `PATH`.
On Linux and macOS, also set `LD_LIBRARY_PATH` or `DYLD_LIBRARY_PATH` when the
native library is outside the default build directory.

## Example

```go
package main

import (
	"fmt"
	"log"

	coolbox "github.com/MeanderingAI/CoolBox/_deliverables/libraries/bindings/go_bindings"
)

func main() {
	method, err := coolbox.NewLinearRegressionFitMethod(1000, 0.01, coolbox.ClosedForm)
	if err != nil {
		log.Fatal(err)
	}
	model, err := coolbox.NewLinearRegression(method)
	if err != nil {
		log.Fatal(err)
	}
	defer model.Close()

	err = model.Fit(
		[][]float64{{1}, {2}, {3}, {4}},
		[]float64{5, 8, 11, 14},
	)
	if err != nil {
		log.Fatal(err)
	}
	prediction, err := model.Predict([]float64{5})
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println(prediction)
}
```

Deep-learning example:

```go
network, err := coolbox.NewNeuralNetwork()
if err != nil {
	log.Fatal(err)
}
defer network.Close()

dense, err := coolbox.NewDenseLayer(2, 1)
if err != nil {
	log.Fatal(err)
}
defer dense.Close()
if err := network.AddLayer(dense); err != nil {
	log.Fatal(err)
}
loss, err := coolbox.NewLoss(coolbox.MeanSquaredError)
if err != nil {
	log.Fatal(err)
}
defer loss.Close()
if err := network.SetLoss(loss); err != nil {
	log.Fatal(err)
}

input, err := coolbox.NewTensorFromData([]int{1, 2}, []float64{0.5, 1.5})
if err != nil {
	log.Fatal(err)
}
defer input.Close()
target, err := coolbox.NewTensorFromData([]int{1, 1}, []float64{2})
if err != nil {
	log.Fatal(err)
}
defer target.Close()
if err := network.Train([]*coolbox.Tensor{input}, []*coolbox.Tensor{target}, 10, 1, false); err != nil {
	log.Fatal(err)
}
prediction, err := network.Predict(input)
if err != nil {
	log.Fatal(err)
}
defer prediction.Close()
```

Plotting example:

```go
plot, err := coolbox.NewGraph(800, 600, coolbox.LineGraph)
if err != nil {
	log.Fatal(err)
}
defer plot.Close()
if err := plot.SetTitle("Training loss"); err != nil {
	log.Fatal(err)
}
if err := plot.AddSeries(coolbox.Series{
	Label: "train",
	X:     []float64{1, 2, 3, 4},
	Y:     []float64{0.9, 0.6, 0.3, 0.1},
	Color: coolbox.Blue,
}); err != nil {
	log.Fatal(err)
}
if err := plot.SavePNG("loss.png"); err != nil {
	log.Fatal(err)
}
```

For a direct 2D or 3D plot, use `Plot2D` or `Plot3D` and close the returned
canvas when finished:

```go
canvas, err := coolbox.Plot2D(
	[]float64{0, 1, 2},
	[]float64{1, 3, 2},
	coolbox.Plot2DOptions{Title: "Measurements"},
)
if err != nil {
	log.Fatal(err)
}
defer canvas.Close()
if err := canvas.SavePNG("measurements.png"); err != nil {
	log.Fatal(err)
}
```

The convenience helpers default to an 800x600 canvas and X/Y (plus Z for 3D)
axis labels. Set `ShowWindow: true` to open the rendered plot in the system
browser with wheel zoom, drag-to-pan, and PNG download controls. This viewer
uses only local temporary HTML and does not upload plot data.

Data-structure example:

```go
tree, err := coolbox.NewInt64BPlusTree()
if err != nil {
	log.Fatal(err)
}
defer tree.Close()

if err := tree.Set(42, 9001); err != nil {
	log.Fatal(err)
}
value, found, err := tree.Get(42)
if err != nil {
	log.Fatal(err)
}
if found {
	fmt.Println(value)
}
```

`Client`, tensor, layer, loss, optimizer, network, HELiX, graph, and canvas
instances and data structures own native allocations; call `Close` when done.
The native neural
network implementation currently applies its own fixed learning rate when
training; configuring an optimizer stores it on the network but does not yet
change that training behavior.

Dense matrix and vector operations have a process-wide backend selector:
`SetMatrixBackend(BackendEigen)`, `SetMatrixBackend(BackendCPU)`, or
`SetMatrixBackend(BackendAuto)`. `MatrixBackendAvailable` checks whether a
backend can really run in this build; `CurrentMatrixBackend` reads the requested
choice and `ActiveMatrixBackend` reports the resolved backend used by the most
recent operation. `BackendAuto` selects an available GPU, otherwise CPU. CUDA
requires compiling the native bridge with nvcc and an installed CUDA runtime
with a device; Metal requires an Objective-C++ build and an available macOS
Metal device, with `metal_backend.mm` and Metal/Foundation frameworks linked.
GPU choices support matrix addition, multiplication, and
transpose. Norms and scalar access remain host-side; unsupported vector
arithmetic, matrix subtraction, and row-wise operations return an error rather
than silently claiming GPU execution. OpenCL remains unavailable until kernels
are implemented. Eigen headers are required to compile the bindings. The build checks
`external/eigen/eigen-3.4.0` in the repository; otherwise supply the include
directory in `CGO_CXXFLAGS` (for example, `CGO_CXXFLAGS="-I/path/to/eigen"`).
