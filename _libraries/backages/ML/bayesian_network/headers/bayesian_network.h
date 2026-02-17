
/**
 * @mainpage BayesianNetwork Library
 *
 * @section usage_examples Usage Examples
 *
 * @subsection cpp_example C++ Example
 * @code{.cpp}
 * #include "ml/bayesian_network/bayesian_network.h"
 * BayesianNetwork bn;
 * int nodeA = bn.add_node("A", {"T", "F"});
 * int nodeB = bn.add_node("B", {"T", "F"});
 * bn.add_edge(nodeA, nodeB);
 * // ... set CPTs, perform inference ...
 * @endcode
 *
 * @subsection python_example Python Example
 * @code{.python}
 * from ml_core.bayesian_network import BayesianNetwork
 * bn = BayesianNetwork()
 * nodeA = bn.add_node("A", ["T", "F"])
 * nodeB = bn.add_node("B", ["T", "F"])
 * bn.add_edge(nodeA, nodeB)
 * # ... set CPTs, perform inference ...
 * @endcode
 *
 * @subsection js_example JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Async usage (MODULARIZE=1, default):
 * createBayesianNetworkModule().then(Module => {
 *     const BayesianNetwork = Module.BayesianNetwork;
 *     const bn = new BayesianNetwork();
 *     const nodeA = bn.add_node("A", ["T", "F"]);
 *     const nodeB = bn.add_node("B", ["T", "F"]);
 *     bn.add_edge(nodeA, nodeB);
 *     // ... set CPTs, perform inference ...
 * });
 * @endcode
 *
 * @subsection js_example_sync JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If bayesian_network.js is loaded and exposes 'Module' globally:
 * const BayesianNetwork = Module.BayesianNetwork;
 * const bn = new BayesianNetwork();
 * const nodeA = bn.add_node("A", ["T", "F"]);
 * const nodeB = bn.add_node("B", ["T", "F"]);
 * bn.add_edge(nodeA, nodeB);
 * // ... set CPTs, perform inference ...
 * // Note: If built with MODULARIZE=1 (default), you must use createBayesianNetworkModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */
#ifndef BAYESIAN_NETWORK_H
#define BAYESIAN_NETWORK_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <Eigen/Dense>
