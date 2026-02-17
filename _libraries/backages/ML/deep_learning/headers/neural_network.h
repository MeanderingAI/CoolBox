#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "layer.h"
#include "loss.h"
#include "optimizer.h"
#include "tensor.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @mainpage NeuralNetwork Library
 *
 * @section usage_examples Usage Examples
 *
 * @subsection cpp_example C++ Example
 * @code{.cpp}
 * #include "ml/deep_learning/neural_network.h"
 * using namespace ml::deep_learning;
 * NeuralNetwork net;
 * // net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 * net.train(inputs, targets, 10, 32, true);
 * auto preds = net.predict(input);
 * @endcode
 *
 * @subsection python_example Python Example
 * @code{.python}
 * from ml_core.deep_learning import NeuralNetwork
 * net = NeuralNetwork()
 * # net.add_layer(...), net.set_loss(...), net.set_optimizer(...)
 * net.train(inputs, targets, 10, 32, True)
 * preds = net.predict(input)
 * @endcode
 *
 * @subsection js_example JavaScript Example (WASM/Emscripten)
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
 * @subsection js_example_sync JavaScript Example (Synchronous, MODULARIZE=0)
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
#endif // NEURAL_NETWORK_H
