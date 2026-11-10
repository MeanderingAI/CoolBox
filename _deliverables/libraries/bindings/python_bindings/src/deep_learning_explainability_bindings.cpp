#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "explainability_facade.h"

namespace py = pybind11;

void bind_deep_learning_explainability(py::module_& parent) {
    using namespace ml::deep_learning;

    py::module_ module =
        parent.def_submodule("explainability", "Neural-network explanations");

    py::class_<ContrastiveExplanation>(module, "ContrastiveExplanation")
        .def_readonly("model_output", &ContrastiveExplanation::model_output)
        .def_readonly("positive_gradient", &ContrastiveExplanation::positive_gradient)
        .def_readonly("negative_gradient", &ContrastiveExplanation::negative_gradient)
        .def_readonly("contrastive_map", &ContrastiveExplanation::contrastive_map)
        .def_readonly("target_class", &ContrastiveExplanation::target_class)
        .def_readonly("foil_class", &ContrastiveExplanation::foil_class);
    py::class_<LimeExplanation>(module, "LimeExplanation")
        .def_readonly("model_output", &LimeExplanation::model_output)
        .def_readonly("feature_importance", &LimeExplanation::feature_importance)
        .def_readonly("intercept", &LimeExplanation::intercept)
        .def_readonly("local_prediction", &LimeExplanation::local_prediction)
        .def_readonly("model_prediction", &LimeExplanation::model_prediction)
        .def_readonly("target_class", &LimeExplanation::target_class)
        .def_readonly("sampled_features", &LimeExplanation::sampled_features);
    py::class_<IntegratedGradientsExplanation>(
        module, "IntegratedGradientsExplanation")
        .def_readonly("model_output",
                      &IntegratedGradientsExplanation::model_output)
        .def_readonly("attribution",
                      &IntegratedGradientsExplanation::attribution)
        .def_readonly("attribution_sum",
                      &IntegratedGradientsExplanation::attribution_sum)
        .def_readonly("target_class",
                      &IntegratedGradientsExplanation::target_class);
    py::class_<ShapExplanation>(module, "ShapExplanation")
        .def_readonly("model_output", &ShapExplanation::model_output)
        .def_readonly("shap_values", &ShapExplanation::shap_values)
        .def_readonly("base_value", &ShapExplanation::base_value)
        .def_readonly("local_prediction", &ShapExplanation::local_prediction)
        .def_readonly("model_prediction", &ShapExplanation::model_prediction)
        .def_readonly("target_class", &ShapExplanation::target_class)
        .def_readonly("sampled_features", &ShapExplanation::sampled_features);

    py::class_<CDeepExExplainer>(module, "CDeepExExplainer")
        .def(py::init<NeuralNetwork&>(), py::keep_alive<1, 2>())
        .def("explain", &CDeepExExplainer::explain,
             py::arg("input"), py::arg("target_class"),
             py::arg("foil_class") = static_cast<size_t>(-1),
             py::arg("normalize") = true)
        .def("explain_lime", &CDeepExExplainer::explain_lime,
             py::arg("input"), py::arg("target_class"),
             py::arg("num_samples") = 128, py::arg("max_features") = 32,
             py::arg("kernel_width") = 0.75, py::arg("ridge") = 1e-3,
             py::arg("seed") = 42)
        .def("explain_integrated_gradients",
             &CDeepExExplainer::explain_integrated_gradients,
             py::arg("input"), py::arg("target_class"), py::arg("steps") = 32,
             py::arg("normalize") = true)
        .def("explain_kernel_shap", &CDeepExExplainer::explain_kernel_shap,
             py::arg("input"), py::arg("target_class"),
             py::arg("num_samples") = 256, py::arg("max_features") = 32,
             py::arg("ridge") = 1e-6, py::arg("seed") = 7);

    py::enum_<ExplanationMethod>(module, "ExplanationMethod")
        .value("CDEEPEX_CONTRASTIVE", ExplanationMethod::CDEEPEX_CONTRASTIVE)
        .value("LIME", ExplanationMethod::LIME)
        .value("INTEGRATED_GRADIENTS", ExplanationMethod::INTEGRATED_GRADIENTS)
        .value("KERNEL_SHAP", ExplanationMethod::KERNEL_SHAP);
    py::class_<ExplainRequest>(module, "ExplainRequest")
        .def(py::init<>())
        .def_readwrite("method", &ExplainRequest::method)
        .def_readwrite("target_class", &ExplainRequest::target_class)
        .def_readwrite("use_default_foil", &ExplainRequest::use_default_foil)
        .def_readwrite("foil_class", &ExplainRequest::foil_class)
        .def_readwrite("normalize", &ExplainRequest::normalize)
        .def_readwrite("lime_num_samples", &ExplainRequest::lime_num_samples)
        .def_readwrite("lime_max_features", &ExplainRequest::lime_max_features)
        .def_readwrite("lime_kernel_width", &ExplainRequest::lime_kernel_width)
        .def_readwrite("lime_ridge", &ExplainRequest::lime_ridge)
        .def_readwrite("lime_seed", &ExplainRequest::lime_seed)
        .def_readwrite("ig_steps", &ExplainRequest::ig_steps)
        .def_readwrite("shap_num_samples", &ExplainRequest::shap_num_samples)
        .def_readwrite("shap_max_features", &ExplainRequest::shap_max_features)
        .def_readwrite("shap_ridge", &ExplainRequest::shap_ridge)
        .def_readwrite("shap_seed", &ExplainRequest::shap_seed)
        .def_static("for_contrastive", &ExplainRequest::for_contrastive,
                    py::arg("target_class"), py::arg("normalize") = true,
                    py::arg("use_default_foil") = true,
                    py::arg("foil_class") = 0)
        .def_static("for_lime", &ExplainRequest::for_lime,
                    py::arg("target_class"), py::arg("num_samples") = 128,
                    py::arg("max_features") = 32,
                    py::arg("kernel_width") = 0.75, py::arg("ridge") = 1e-3,
                    py::arg("seed") = 42)
        .def_static("for_integrated_gradients",
                    &ExplainRequest::for_integrated_gradients,
                    py::arg("target_class"), py::arg("steps") = 32,
                    py::arg("normalize") = true)
        .def_static("for_kernel_shap", &ExplainRequest::for_kernel_shap,
                    py::arg("target_class"), py::arg("num_samples") = 256,
                    py::arg("max_features") = 32, py::arg("ridge") = 1e-6,
                    py::arg("seed") = 7);
    py::class_<ExplainabilityResult>(module, "ExplainabilityResult")
        .def_readonly("method", &ExplainabilityResult::method)
        .def_readonly("contrastive", &ExplainabilityResult::contrastive)
        .def_readonly("lime", &ExplainabilityResult::lime)
        .def_readonly("integrated_gradients",
                      &ExplainabilityResult::integrated_gradients)
        .def_readonly("shap", &ExplainabilityResult::shap);
    py::class_<ExplainabilityFacade>(module, "ExplainabilityFacade")
        .def(py::init<NeuralNetwork&>(), py::keep_alive<1, 2>())
        .def("explain", &ExplainabilityFacade::explain);
}
