#ifndef EXPLAINABILITY_FACADE_H
#define EXPLAINABILITY_FACADE_H

#include "cdeepex.h"

namespace ml {
namespace deep_learning {

enum class ExplanationMethod {
    CDEEPEX_CONTRASTIVE,
    LIME,
    INTEGRATED_GRADIENTS,
    KERNEL_SHAP
};

struct ExplainRequest {
    ExplanationMethod method = ExplanationMethod::CDEEPEX_CONTRASTIVE;
    size_t target_class = 0;

    // Contrastive settings
    bool use_default_foil = true;
    size_t foil_class = 0;
    bool normalize = true;

    // LIME settings
    size_t lime_num_samples = 128;
    size_t lime_max_features = 32;
    double lime_kernel_width = 0.75;
    double lime_ridge = 1e-3;
    unsigned int lime_seed = 42;

    // Integrated gradients settings
    size_t ig_steps = 32;

    // KernelSHAP settings
    size_t shap_num_samples = 256;
    size_t shap_max_features = 32;
    double shap_ridge = 1e-6;
    unsigned int shap_seed = 7;

    static ExplainRequest for_contrastive(size_t target_class,
                                          bool normalize = true,
                                          bool use_default_foil = true,
                                          size_t foil_class = 0) {
        ExplainRequest r;
        r.method = ExplanationMethod::CDEEPEX_CONTRASTIVE;
        r.target_class = target_class;
        r.normalize = normalize;
        r.use_default_foil = use_default_foil;
        r.foil_class = foil_class;
        return r;
    }

    static ExplainRequest for_lime(size_t target_class,
                                   size_t num_samples = 128,
                                   size_t max_features = 32,
                                   double kernel_width = 0.75,
                                   double ridge = 1e-3,
                                   unsigned int seed = 42) {
        ExplainRequest r;
        r.method = ExplanationMethod::LIME;
        r.target_class = target_class;
        r.lime_num_samples = num_samples;
        r.lime_max_features = max_features;
        r.lime_kernel_width = kernel_width;
        r.lime_ridge = ridge;
        r.lime_seed = seed;
        return r;
    }

    static ExplainRequest for_integrated_gradients(size_t target_class,
                                                   size_t steps = 32,
                                                   bool normalize = true) {
        ExplainRequest r;
        r.method = ExplanationMethod::INTEGRATED_GRADIENTS;
        r.target_class = target_class;
        r.ig_steps = steps;
        r.normalize = normalize;
        return r;
    }

    static ExplainRequest for_kernel_shap(size_t target_class,
                                          size_t num_samples = 256,
                                          size_t max_features = 32,
                                          double ridge = 1e-6,
                                          unsigned int seed = 7) {
        ExplainRequest r;
        r.method = ExplanationMethod::KERNEL_SHAP;
        r.target_class = target_class;
        r.shap_num_samples = num_samples;
        r.shap_max_features = max_features;
        r.shap_ridge = ridge;
        r.shap_seed = seed;
        return r;
    }
};

struct ExplainabilityResult {
    ExplanationMethod method = ExplanationMethod::CDEEPEX_CONTRASTIVE;
    ContrastiveExplanation contrastive;
    LimeExplanation lime;
    IntegratedGradientsExplanation integrated_gradients;
    ShapExplanation shap;
};

class ExplainabilityFacade {
public:
    explicit ExplainabilityFacade(NeuralNetwork& network);

    ExplainabilityResult explain(const Tensor& input, const ExplainRequest& request);

private:
    CDeepExExplainer explainer_;
};

} // namespace deep_learning
} // namespace ml

#endif // EXPLAINABILITY_FACADE_H
