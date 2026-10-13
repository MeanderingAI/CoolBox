#ifndef CDEEPEX_H
#define CDEEPEX_H

#include "neural_network.h"
#include <cstddef>
#include <vector>

namespace ml {
namespace deep_learning {

struct ContrastiveExplanation {
    Tensor model_output;
    Tensor positive_gradient;
    Tensor negative_gradient;
    Tensor contrastive_map;
    size_t target_class;
    size_t foil_class;
};

struct LimeExplanation {
    Tensor model_output;
    Tensor feature_importance;
    double intercept;
    double local_prediction;
    double model_prediction;
    size_t target_class;
    size_t sampled_features;
};

struct IntegratedGradientsExplanation {
    Tensor model_output;
    Tensor attribution;
    double attribution_sum;
    size_t target_class;
};

struct ShapExplanation {
    Tensor model_output;
    Tensor shap_values;
    double base_value;
    double local_prediction;
    double model_prediction;
    size_t target_class;
    size_t sampled_features;
};

class CDeepExExplainer {
public:
    explicit CDeepExExplainer(NeuralNetwork& network) : network_(network) {}

    // CDeepEx-style contrastive explanation at input level:
    // positive evidence for target class contrasted against a foil class.
    // If foil_class is SIZE_MAX, the strongest non-target class is used.
    ContrastiveExplanation explain(const Tensor& input,
                                   size_t target_class,
                                   size_t foil_class = static_cast<size_t>(-1),
                                   bool normalize = true);

    // LIME-style local linear surrogate around input.
    LimeExplanation explain_lime(const Tensor& input,
                                 size_t target_class,
                                 size_t num_samples = 128,
                                 size_t max_features = 32,
                                 double kernel_width = 0.75,
                                 double ridge = 1e-3,
                                 unsigned int seed = 42);

    // Integrated Gradients attribution from a zero baseline.
    IntegratedGradientsExplanation explain_integrated_gradients(
        const Tensor& input,
        size_t target_class,
        size_t steps = 32,
        bool normalize = true);

    // KernelSHAP-style local additive attribution.
    ShapExplanation explain_kernel_shap(const Tensor& input,
                                        size_t target_class,
                                        size_t num_samples = 256,
                                        size_t max_features = 32,
                                        double ridge = 1e-6,
                                        unsigned int seed = 7);

private:
    NeuralNetwork& network_;

    static size_t class_count(const Tensor& output);
    static size_t class_offset(const Tensor& output, size_t class_index);
    static size_t pick_default_foil(const Tensor& output, size_t target_class);
    static Tensor seeded_output_gradient(const Tensor& output, size_t class_index);
    static Tensor relu_abs_difference(const Tensor& positive, const Tensor& negative);
    static Tensor min_max_normalize(const Tensor& input);
    static std::vector<size_t> top_feature_indices(const Tensor& input, size_t max_features);
    static std::vector<double> solve_weighted_ridge(const std::vector<std::vector<double>>& X,
                                                    const std::vector<double>& y,
                                                    const std::vector<double>& w,
                                                    double ridge);
    static double target_score(const Tensor& output, size_t class_index);
    static double binomial_coeff(size_t n, size_t k);
};

} // namespace deep_learning
} // namespace ml

#endif // CDEEPEX_H
