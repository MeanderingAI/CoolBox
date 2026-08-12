#include "explainability_facade.h"

#include <stdexcept>

namespace ml {
namespace deep_learning {

ExplainabilityFacade::ExplainabilityFacade(NeuralNetwork& network)
    : explainer_(network) {}

ExplainabilityResult ExplainabilityFacade::explain(const Tensor& input, const ExplainRequest& request) {
    ExplainabilityResult result;
    result.method = request.method;

    switch (request.method) {
        case ExplanationMethod::CDEEPEX_CONTRASTIVE: {
            const size_t foil = request.use_default_foil
                ? static_cast<size_t>(-1)
                : request.foil_class;
            result.contrastive = explainer_.explain(
                input,
                request.target_class,
                foil,
                request.normalize);
            break;
        }
        case ExplanationMethod::LIME: {
            result.lime = explainer_.explain_lime(
                input,
                request.target_class,
                request.lime_num_samples,
                request.lime_max_features,
                request.lime_kernel_width,
                request.lime_ridge,
                request.lime_seed);
            break;
        }
        case ExplanationMethod::INTEGRATED_GRADIENTS: {
            result.integrated_gradients = explainer_.explain_integrated_gradients(
                input,
                request.target_class,
                request.ig_steps,
                request.normalize);
            break;
        }
        case ExplanationMethod::KERNEL_SHAP: {
            result.shap = explainer_.explain_kernel_shap(
                input,
                request.target_class,
                request.shap_num_samples,
                request.shap_max_features,
                request.shap_ridge,
                request.shap_seed);
            break;
        }
        default:
            throw std::invalid_argument("unsupported explanation method");
    }

    return result;
}

} // namespace deep_learning
} // namespace ml
