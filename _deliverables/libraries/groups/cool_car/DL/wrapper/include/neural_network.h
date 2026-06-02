#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "layer.h"
#include "loss.h"
#include "optimizer.h"
#include "../../layers/include/tensor.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @page neural_network_main NeuralNetwork Library
 *
 * @section usage_examples_neural_network Usage Examples
 *
 * @subsection cpp_example_neural_network C++ Example
 * @code{.cpp}
 * #include "ml/deep_learning/neural_network.h"
 * using namespace ml::deep_learning;
 * NeuralNetwork net;
 * // net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 * net.train(inputs, targets, 10, 32, true);
 * auto preds = net.predict(input);
 * @endcode
 *
 * @subsection python_example_neural_network Python Example
 * @code{.python}
 * from ml_core.deep_learning import NeuralNetwork
 * net = NeuralNetwork()
 * # net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 * net.train(inputs, targets, 10, 32, True)
 * preds = net.predict(input)
 * @endcode
 *
 * @subsection js_example_neural_network JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Async usage (MODULARIZE=1, default):
 * createNeuralNetworkModule().then(Module => {
 *     const NeuralNetwork = Module.NeuralNetwork;
 *     const net = new NeuralNetwork();
 *     // net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 *     net.train(inputs, targets, 10, 32, true);
 *     const preds = net.predict(input);
 * });
 * @endcode
 *
 * @subsection js_example_sync_neural_network JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If neural_network.js is loaded and exposes 'Module' globally:
 * const NeuralNetwork = Module.NeuralNetwork;
 * const net = new NeuralNetwork();
 * // net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 * net.train(inputs, targets, 10, 32, true);
 * const preds = net.predict(input);
 * // Note: If built with MODULARIZE=1 (default), you must use createNeuralNetworkModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */

namespace ml {
namespace deep_learning {

class NeuralNetwork {
public:
    NeuralNetwork();

    void add_layer(std::shared_ptr<Layer> layer);
    void set_loss(std::shared_ptr<Loss> loss);
    void set_optimizer(std::shared_ptr<Optimizer> optimizer);

    Tensor forward(const Tensor& input);
    void train(const std::vector<Tensor>& inputs, const std::vector<Tensor>& targets,
               int epochs = 10, int batch_size = 32, bool verbose = false);
    Tensor predict(const Tensor& input);

    double train_step(const Tensor& input, const Tensor& target);
    double evaluate(const std::vector<Tensor>& inputs, const std::vector<Tensor>& targets);

    void set_training(bool training);
    void summary() const;

    double get_last_loss() const { return last_loss_; }
    size_t num_layers() const { return layers_.size(); }

private:
    std::vector<std::shared_ptr<Layer>> layers_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Optimizer> optimizer_;
    double last_loss_ = 0.0;
    bool training_ = true;

    void backward(const Tensor& loss_gradient);
    void update_parameters();
};

} // namespace deep_learning
} // namespace ml

#endif // NEURAL_NETWORK_H
