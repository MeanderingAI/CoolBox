#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <tree_ensemble/boost_tree.h>
#include <tree_ensemble/random_forest_regressor.h>

namespace py = pybind11;

void bind_tree_ensembles(py::module_& module) {
    py::class_<RandomForestRegressor>(module, "RandomForestRegressor")
        .def(py::init<int, int, unsigned int>(),
             py::arg("num_trees"), py::arg("max_depth"), py::arg("seed") = 5489u)
        .def("fit", &RandomForestRegressor::fit, py::arg("X"), py::arg("y"))
        .def("predict", &RandomForestRegressor::predict, py::arg("sample"));

    py::class_<BoostTreeParameters>(module, "BoostTreeParameters")
        .def(py::init<>())
        .def_readwrite("num_estimators", &BoostTreeParameters::num_estimators)
        .def_readwrite("learning_rate", &BoostTreeParameters::learning_rate)
        .def_readwrite("max_depth", &BoostTreeParameters::max_depth);

    py::class_<BoostTree>(module, "BoostTree")
        .def(py::init<const BoostTreeParameters&>(), py::arg("params") = BoostTreeParameters())
        .def("fit", &BoostTree::fit, py::arg("X"), py::arg("y"))
        .def("predict", py::overload_cast<const std::vector<double>&>(&BoostTree::predict, py::const_),
             py::arg("sample"))
        .def("predict_batch",
             py::overload_cast<const std::vector<std::vector<double>>&>(&BoostTree::predict, py::const_),
             py::arg("X"));

    py::class_<BoostTreeClassifier>(module, "BoostTreeClassifier")
        .def(py::init<const BoostTreeParameters&>(), py::arg("params") = BoostTreeParameters())
        .def("fit", &BoostTreeClassifier::fit, py::arg("X"), py::arg("y"))
        .def("predict", &BoostTreeClassifier::predict, py::arg("sample"))
        .def("predict_proba", &BoostTreeClassifier::predict_proba, py::arg("sample"))
        .def("classes", &BoostTreeClassifier::classes);
}
