#include <emscripten/bind.h>
#include "bayesian_network.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(bayesian_network_module) {
    register_vector<std::string>("VectorString_BN");
    register_vector<double>("VectorDouble_BN");
    register_vector<int>("VectorInt_BN");

    class_<BayesianNetwork>("BayesianNetwork")
        .constructor<>()
        .function("add_node", &BayesianNetwork::add_node)
        .function("add_edge", select_overload<void(int, int)>(&BayesianNetwork::add_edge))
        .function("set_cpt", select_overload<void(int, const std::vector<double>&)>(&BayesianNetwork::set_cpt))
        .function("num_nodes", &BayesianNetwork::num_nodes)
        .function("get_node_id", &BayesianNetwork::get_node_id)
    ;
}
