#ifndef ML_DEEP_LEARNING_SCALING_MOE_H
#define ML_DEEP_LEARNING_SCALING_MOE_H

#include "layer.h"
#include "tensor.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Mixture-of-Experts training and inference building blocks.
///
/// Implements the gating, capacity, Pyramid-Residual-MoE architecture, expert
/// slicing and staged Mixture-of-Students distillation of
/// Rajbhandari et al., "DeepSpeed-MoE: Advancing Mixture-of-Experts Inference
/// and Training to Power Next-Generation AI Scale", ICML 2022
/// (arXiv:2201.05596), citation key @c rajbhandari_deepspeed_moe_2022.
struct MoEConfig {
    size_t d_model = 0;
    size_t d_ff = 0;
    size_t num_experts = 2;
    size_t top_k = 1;
    /// Expert buffer size is capacity_factor * tokens / num_experts.
    double capacity_factor = 1.0;
    /// PR-MoE: adds a always-on dense branch beside the routed experts, which
    /// lets the model keep quality with roughly half the experts.
    bool residual_mlp = false;
    unsigned int seed = 42;
};

/// One token/expert pair chosen by the gate.
struct RoutingSlot {
    size_t token = 0;
    size_t expert = 0;
    double gate_weight = 0.0;
};

class MoELayer : public Layer {
public:
    explicit MoELayer(MoEConfig config);

    /// @param input Token batch shaped [tokens, d_model].
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;

    std::string name() const override { return "MoE"; }
    bool has_parameters() const override { return true; }

    const MoEConfig& config() const { return config_; }
    /// Expert buffer size derived from the capacity factor.
    size_t expert_capacity(size_t tokens) const;
    /// Routing decisions of the last forward pass.
    const std::vector<RoutingSlot>& routing() const { return routing_; }
    /// Tokens the gate selected but that overflowed their expert's capacity.
    size_t dropped_tokens() const { return dropped_tokens_; }
    /// Auxiliary load-balancing loss, E * sum_e fraction_e * mean_probability_e.
    double load_balancing_loss() const { return load_balancing_loss_; }
    /// Parameters activated per token versus total parameters.
    double activation_sparsity() const;
    size_t total_parameters() const;
    size_t active_parameters_per_token() const;

private:
    struct Expert {
        Tensor w1, b1, w2, b2;
        Tensor gw1, gb1, gw2, gb2;
    };

    struct SlotCache {
        size_t token = 0;
        size_t expert = 0;
        double gate_weight = 0.0;
        std::vector<double> hidden;
        std::vector<double> activated;
        std::vector<double> output;
    };

    void zero_gradients();

    MoEConfig config_;
    std::vector<Expert> experts_;
    Tensor gate_weights_;     ///< [d_model, num_experts]
    Tensor gate_gradient_;
    Tensor residual_w_, residual_b_;
    Tensor residual_gw_, residual_gb_;

    Tensor gate_probabilities_; ///< [tokens, num_experts] from the last forward.
    std::vector<SlotCache> cache_;
    std::vector<RoutingSlot> routing_;
    size_t dropped_tokens_ = 0;
    double load_balancing_loss_ = 0.0;
};

/// Pyramid-Residual MoE: later layers get more experts because that is where
/// the extra capacity pays off. Returns one expert count per layer.
std::vector<size_t> pyramid_expert_counts(size_t num_layers, size_t first_layer_experts,
                                          size_t last_layer_experts);

/// Expert-parallel placement: which experts live on which device.
std::vector<std::vector<size_t>> expert_placement(size_t num_experts, size_t expert_parallel_degree);

/// One phase of the staged Mixture-of-Students knowledge-distillation schedule.
struct DistillationStage {
    size_t start_step = 0;
    size_t end_step = 0;
    double kd_weight = 0.0;
};

/// KD loss weight decays to zero over the first @p stages phases, after which
/// the student trains on the task loss alone.
std::vector<DistillationStage> staged_distillation_schedule(size_t total_steps,
                                                            size_t stages,
                                                            double initial_kd_weight);

/// Combined student objective for a given step of the schedule.
double distillation_loss(double student_task_loss,
                         double teacher_student_divergence,
                         const std::vector<DistillationStage>& schedule,
                         size_t step);

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_MOE_H
