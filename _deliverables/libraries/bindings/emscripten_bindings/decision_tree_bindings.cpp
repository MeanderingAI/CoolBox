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

    enum_<SplitCriterion>("SplitCriterion")
        .value("GINI", SplitCriterion::GINI)
        .value("ENTROPY", SplitCriterion::ENTROPY)
    ;

    class_<DecisionTree>("DecisionTree")
        .constructor<>()
        .constructor<SplitCriterion>()
        .function("fit", &DecisionTree::fit)
        .function("predict", &DecisionTree::predict)
    ;

    class_<DecisionTreeRegressor>("DecisionTreeRegressor")
        .constructor<>()
        .function("fit", &DecisionTreeRegressor::fit)
        .function("predict", &DecisionTreeRegressor::predict)
    ;

    class_<RandomForest>("RandomForest")
        .constructor<int, int>()
        .function("fit", &RandomForest::fit)
        .function("predict", &RandomForest::predict)
    ;

    class_<RandomForestRegressor>("RandomForestRegressor")
        .constructor<int, int>()
        .constructor<int, int, unsigned int>()
        .function("fit", &RandomForestRegressor::fit)
        .function("predict", &RandomForestRegressor::predict)
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

    class_<BoostTreeClassifier>("BoostTreeClassifier")
        .constructor<>()
        .constructor<BoostTreeParameters>()
        .function("fit", &BoostTreeClassifier::fit)
        .function("predict", &BoostTreeClassifier::predict)
        .function("predict_proba", &BoostTreeClassifier::predict_proba)
        .function("classes", optional_override([](const BoostTreeClassifier& model) {
            return model.classes();
        }))
    ;
}
