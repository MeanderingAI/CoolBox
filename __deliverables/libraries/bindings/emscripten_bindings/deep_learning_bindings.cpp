// Deep learning bindings — NeuralNetwork API uses Eigen types internally.
// Expose Tensor creation and basic ops.
#include <emscripten/bind.h>
#include "tensor.h"

using namespace emscripten;
using namespace ml::deep_learning;

EMSCRIPTEN_BINDINGS(deep_learning_module) {
    class_<Tensor>("Tensor")
        .constructor<>()
        .function("fill", &Tensor::fill)
        .function("size", &Tensor::size)
        .function("sum", &Tensor::sum)
        .function("mean", &Tensor::mean)
    ;
}
