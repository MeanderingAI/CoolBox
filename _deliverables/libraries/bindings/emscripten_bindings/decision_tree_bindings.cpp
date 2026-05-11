#include <emscripten/bind.h>
#include "decision_tree.h"
#include "random_forest.h"
#include "boost_tree.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(decision_tree_module) {
    register_vector<int>("VectorInt_DT");
    register_vector<double>("VectorDouble_DT");
    register_vector<std::vector<int>>("VectorVectorInt_DT");
    register_vector<std::vector<double>>("VectorVectorDouble_DT");

    class_<DecisionTree>("DecisionTree")
        .constructor<>()
        .function("fit", &DecisionTree::fit)
        .function("predict", &DecisionTree::predict)
    ;

    class_<RandomForest>("RandomForest")
        .constructor<int, int>()
        .function("fit", &RandomForest::fit)
        .function("predict", &RandomForest::predict)
    ;

    value_object<BoostTreeParameters>("BoostTreeParameters")
        .field("num_estimators", &BoostTreeParameters::num_estimators)
        .field("learning_rate", &BoostTreeParameters::learning_rate)
        .field("max_depth", &BoostTreeParameters::max_depth)
    ;

    class_<BoostTree>("BoostTree")
        .constructor<BoostTreeParameters>()
        .function("fit", &BoostTree::fit)
        .function("predict", optional_override([](const BoostTree& bt, const std::vector<double>& sample) {
            return bt.predict(sample);
        }))
    ;
}
