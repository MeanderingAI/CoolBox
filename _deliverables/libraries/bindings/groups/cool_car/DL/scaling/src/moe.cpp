#include "moe.h"

#include "collective.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

void xavier_init(Tensor& tensor, size_t fan_in, size_t fan_out, std::mt19937& generator) {
    const double limit = std::sqrt(6.0 / static_cast<double>(fan_in + fan_out));
    std::uniform_real_distribution<double> distribution(-limit, limit);
    for (auto& value : tensor.data()) {
        value = distribution(generator);
    }
}

} // namespace

MoELayer::MoELayer(MoEConfig config) : config_(config) {
    if (config_.d_model == 0 || config_.d_ff == 0) {
        throw std::invalid_argument("MoELayer: d_model and d_ff must be > 0");
    }
    if (config_.num_experts == 0) {
        throw std::invalid_argument("MoELayer: num_experts must be > 0");
    }
    if (config_.top_k == 0 || config_.top_k > config_.num_experts) {
        throw std::invalid_argument("MoELayer: top_k must be in [1, num_experts]");
    }
    if (config_.capacity_factor <= 0.0) {
        throw std::invalid_argument("MoELayer: capacity_factor must be > 0");
    }

    std::mt19937 generator(config_.seed);

    experts_.resize(config_.num_experts);
    for (auto& expert : experts_) {
        expert.w1 = Tensor({config_.d_model, config_.d_ff});
        expert.b1 = Tensor({config_.d_ff}, 0.0);
        expert.w2 = Tensor({config_.d_ff, config_.d_model});
        expert.b2 = Tensor({config_.d_model}, 0.0);
        xavier_init(expert.w1, config_.d_model, config_.d_ff, generator);
        xavier_init(expert.w2, config_.d_ff, config_.d_model, generator);
        expert.gw1 = Tensor({config_.d_model, config_.d_ff}, 0.0);
        expert.gb1 = Tensor({config_.d_ff}, 0.0);
        expert.gw2 = Tensor({config_.d_ff, config_.d_model}, 0.0);
        expert.gb2 = Tensor({config_.d_model}, 0.0);
    }

    gate_weights_ = Tensor({config_.d_model, config_.num_experts});
    xavier_init(gate_weights_, config_.d_model, config_.num_experts, generator);
    gate_gradient_ = Tensor({config_.d_model, config_.num_experts}, 0.0);

    if (config_.residual_mlp) {
        residual_w_ = Tensor({config_.d_model, config_.d_model});
        residual_b_ = Tensor({config_.d_model}, 0.0);
        xavier_init(residual_w_, config_.d_model, config_.d_model, generator);
        residual_gw_ = Tensor({config_.d_model, config_.d_model}, 0.0);
        residual_gb_ = Tensor({config_.d_model}, 0.0);
    }
}

size_t MoELayer::expert_capacity(size_t tokens) const {
    const double raw = config_.capacity_factor * static_cast<double>(tokens * config_.top_k) /
                       static_cast<double>(config_.num_experts);
    return std::max<size_t>(1, static_cast<size_t>(std::ceil(raw)));
}

size_t MoELayer::total_parameters() const {
    const size_t per_expert = config_.d_model * config_.d_ff + config_.d_ff +
                              config_.d_ff * config_.d_model + config_.d_model;
    size_t total = per_expert * config_.num_experts + config_.d_model * config_.num_experts;
    if (config_.residual_mlp) {
        total += config_.d_model * config_.d_model + config_.d_model;
    }
    return total;
}

size_t MoELayer::active_parameters_per_token() const {
    const size_t per_expert = config_.d_model * config_.d_ff + config_.d_ff +
                              config_.d_ff * config_.d_model + config_.d_model;
    size_t active = per_expert * config_.top_k + config_.d_model * config_.num_experts;
    if (config_.residual_mlp) {
        active += config_.d_model * config_.d_model + config_.d_model;
    }
    return active;
}

double MoELayer::activation_sparsity() const {
    return static_cast<double>(active_parameters_per_token()) /
           static_cast<double>(total_parameters());
}

void MoELayer::zero_gradients() {
    for (auto& expert : experts_) {
        expert.gw1.fill(0.0);
        expert.gb1.fill(0.0);
        expert.gw2.fill(0.0);
        expert.gb2.fill(0.0);
    }
    gate_gradient_.fill(0.0);
    if (config_.residual_mlp) {
        residual_gw_.fill(0.0);
        residual_gb_.fill(0.0);
    }
}

Tensor MoELayer::forward(const Tensor& input) {
    if (input.shape().size() != 2 || input.shape()[1] != config_.d_model) {
        throw std::invalid_argument("MoELayer::forward: input must be [tokens, d_model]");
    }
    last_input_ = input;

    const size_t tokens = input.shape()[0];
    const size_t d_model = config_.d_model;
    const size_t d_ff = config_.d_ff;
    const size_t num_experts = config_.num_experts;

    // Gate: softmax over the expert logits of each token.
    gate_probabilities_ = Tensor({tokens, num_experts}, 0.0);
    for (size_t t = 0; t < tokens; ++t) {
        double max_logit = -std::numeric_limits<double>::infinity();
        for (size_t e = 0; e < num_experts; ++e) {
            double logit = 0.0;
            for (size_t d = 0; d < d_model; ++d) {
                logit += input.data()[t * d_model + d] * gate_weights_.data()[d * num_experts + e];
            }
            gate_probabilities_.data()[t * num_experts + e] = logit;
            max_logit = std::max(max_logit, logit);
        }
        double sum = 0.0;
        for (size_t e = 0; e < num_experts; ++e) {
            double& value = gate_probabilities_.data()[t * num_experts + e];
            value = std::exp(value - max_logit);
            sum += value;
        }
        for (size_t e = 0; e < num_experts; ++e) {
            gate_probabilities_.data()[t * num_experts + e] /= sum;
        }
    }

    const size_t capacity = expert_capacity(tokens);
    std::vector<size_t> expert_load(num_experts, 0);
    std::vector<size_t> routed_count(num_experts, 0);

    cache_.clear();
    routing_.clear();
    dropped_tokens_ = 0;

    Tensor output({tokens, d_model}, 0.0);

    for (size_t t = 0; t < tokens; ++t) {
        std::vector<size_t> order(num_experts);
        std::iota(order.begin(), order.end(), 0);
        std::partial_sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(config_.top_k),
                          order.end(), [&](size_t a, size_t b) {
            return gate_probabilities_.data()[t * num_experts + a] >
                   gate_probabilities_.data()[t * num_experts + b];
        });

        for (size_t slot = 0; slot < config_.top_k; ++slot) {
            const size_t e = order[slot];
            const double gate = gate_probabilities_.data()[t * num_experts + e];
            routed_count[e] += 1;
            if (expert_load[e] >= capacity) {
                // Capacity overflow: the token skips this expert entirely.
                ++dropped_tokens_;
                continue;
            }
            ++expert_load[e];
            routing_.push_back(RoutingSlot{t, e, gate});

            const Expert& expert = experts_[e];
            SlotCache entry;
            entry.token = t;
            entry.expert = e;
            entry.gate_weight = gate;
            entry.hidden.assign(d_ff, 0.0);
            entry.activated.assign(d_ff, 0.0);
            entry.output.assign(d_model, 0.0);

            for (size_t j = 0; j < d_ff; ++j) {
                double sum = expert.b1.data()[j];
                for (size_t d = 0; d < d_model; ++d) {
                    sum += input.data()[t * d_model + d] * expert.w1.data()[d * d_ff + j];
                }
                entry.hidden[j] = sum;
                entry.activated[j] = std::max(0.0, sum);
            }
            for (size_t d = 0; d < d_model; ++d) {
                double sum = expert.b2.data()[d];
                for (size_t j = 0; j < d_ff; ++j) {
                    sum += entry.activated[j] * expert.w2.data()[j * d_model + d];
                }
                entry.output[d] = sum;
                output.data()[t * d_model + d] += gate * sum;
            }
            cache_.push_back(std::move(entry));
        }
    }

    if (config_.residual_mlp) {
        for (size_t t = 0; t < tokens; ++t) {
            for (size_t o = 0; o < d_model; ++o) {
                double sum = residual_b_.data()[o];
                for (size_t d = 0; d < d_model; ++d) {
                    sum += input.data()[t * d_model + d] * residual_w_.data()[d * d_model + o];
                }
                output.data()[t * d_model + o] += sum;
            }
        }
    }

    // Switch-Transformer style auxiliary loss, as used by DeepSpeed-MoE.
    load_balancing_loss_ = 0.0;
    if (tokens > 0) {
        for (size_t e = 0; e < num_experts; ++e) {
            const double fraction = static_cast<double>(routed_count[e]) /
                                    static_cast<double>(tokens * config_.top_k);
            double mean_probability = 0.0;
            for (size_t t = 0; t < tokens; ++t) {
                mean_probability += gate_probabilities_.data()[t * num_experts + e];
            }
            mean_probability /= static_cast<double>(tokens);
            load_balancing_loss_ += fraction * mean_probability;
        }
        load_balancing_loss_ *= static_cast<double>(num_experts);
    }

    last_output_ = output;
    return output;
}

Tensor MoELayer::backward(const Tensor& gradient) {
    if (gradient.shape() != last_output_.shape()) {
        throw std::invalid_argument("MoELayer::backward: gradient shape must match the output");
    }

    const size_t tokens = last_input_.shape()[0];
    const size_t d_model = config_.d_model;
    const size_t d_ff = config_.d_ff;
    const size_t num_experts = config_.num_experts;

    zero_gradients();
    Tensor input_gradient({tokens, d_model}, 0.0);
    Tensor probability_gradient({tokens, num_experts}, 0.0);

    for (const SlotCache& entry : cache_) {
        Expert& expert = experts_[entry.expert];
        const size_t t = entry.token;

        // Gate weight is a scalar multiplier on this expert's output.
        double gate_grad = 0.0;
        for (size_t d = 0; d < d_model; ++d) {
            gate_grad += gradient.data()[t * d_model + d] * entry.output[d];
        }
        probability_gradient.data()[t * num_experts + entry.expert] += gate_grad;

        std::vector<double> d_output(d_model);
        for (size_t d = 0; d < d_model; ++d) {
            d_output[d] = gradient.data()[t * d_model + d] * entry.gate_weight;
            expert.gb2.data()[d] += d_output[d];
        }

        std::vector<double> d_activated(d_ff, 0.0);
        for (size_t j = 0; j < d_ff; ++j) {
            for (size_t d = 0; d < d_model; ++d) {
                expert.gw2.data()[j * d_model + d] += entry.activated[j] * d_output[d];
                d_activated[j] += expert.w2.data()[j * d_model + d] * d_output[d];
            }
        }

        std::vector<double> d_hidden(d_ff, 0.0);
        for (size_t j = 0; j < d_ff; ++j) {
            d_hidden[j] = entry.hidden[j] > 0.0 ? d_activated[j] : 0.0;
            expert.gb1.data()[j] += d_hidden[j];
        }

        for (size_t d = 0; d < d_model; ++d) {
            const double x = last_input_.data()[t * d_model + d];
            double accumulated = 0.0;
            for (size_t j = 0; j < d_ff; ++j) {
                expert.gw1.data()[d * d_ff + j] += x * d_hidden[j];
                accumulated += expert.w1.data()[d * d_ff + j] * d_hidden[j];
            }
            input_gradient.data()[t * d_model + d] += accumulated;
        }
    }

    if (config_.residual_mlp) {
        for (size_t t = 0; t < tokens; ++t) {
            for (size_t o = 0; o < d_model; ++o) {
                const double g = gradient.data()[t * d_model + o];
                residual_gb_.data()[o] += g;
                for (size_t d = 0; d < d_model; ++d) {
                    residual_gw_.data()[d * d_model + o] += last_input_.data()[t * d_model + d] * g;
                    input_gradient.data()[t * d_model + d] += residual_w_.data()[d * d_model + o] * g;
                }
            }
        }
    }

    // Softmax backward for the gate, then into the gate projection.
    for (size_t t = 0; t < tokens; ++t) {
        double dot = 0.0;
        for (size_t e = 0; e < num_experts; ++e) {
            dot += probability_gradient.data()[t * num_experts + e] *
                   gate_probabilities_.data()[t * num_experts + e];
        }
        for (size_t e = 0; e < num_experts; ++e) {
            const double p = gate_probabilities_.data()[t * num_experts + e];
            const double d_logit = p * (probability_gradient.data()[t * num_experts + e] - dot);
            for (size_t d = 0; d < d_model; ++d) {
                gate_gradient_.data()[d * num_experts + e] +=
                    last_input_.data()[t * d_model + d] * d_logit;
                input_gradient.data()[t * d_model + d] +=
                    gate_weights_.data()[d * num_experts + e] * d_logit;
            }
        }
    }

    return input_gradient;
}

void MoELayer::update_parameters(double learning_rate) {
    for (auto& expert : experts_) {
        for (size_t i = 0; i < expert.w1.size(); ++i) {
            expert.w1.data()[i] -= learning_rate * expert.gw1.data()[i];
        }
        for (size_t i = 0; i < expert.b1.size(); ++i) {
            expert.b1.data()[i] -= learning_rate * expert.gb1.data()[i];
        }
        for (size_t i = 0; i < expert.w2.size(); ++i) {
            expert.w2.data()[i] -= learning_rate * expert.gw2.data()[i];
        }
        for (size_t i = 0; i < expert.b2.size(); ++i) {
            expert.b2.data()[i] -= learning_rate * expert.gb2.data()[i];
        }
    }
    for (size_t i = 0; i < gate_weights_.size(); ++i) {
        gate_weights_.data()[i] -= learning_rate * gate_gradient_.data()[i];
    }
    if (config_.residual_mlp) {
        for (size_t i = 0; i < residual_w_.size(); ++i) {
            residual_w_.data()[i] -= learning_rate * residual_gw_.data()[i];
        }
        for (size_t i = 0; i < residual_b_.size(); ++i) {
            residual_b_.data()[i] -= learning_rate * residual_gb_.data()[i];
        }
    }
}

std::vector<size_t> pyramid_expert_counts(size_t num_layers, size_t first_layer_experts,
                                          size_t last_layer_experts) {
    if (num_layers == 0) {
        return {};
    }
    if (num_layers == 1) {
        return {last_layer_experts};
    }

    std::vector<size_t> counts(num_layers);
    const double span = static_cast<double>(num_layers - 1);
    for (size_t layer = 0; layer < num_layers; ++layer) {
        const double t = static_cast<double>(layer) / span;
        const double value = static_cast<double>(first_layer_experts) +
                             t * (static_cast<double>(last_layer_experts) -
                                  static_cast<double>(first_layer_experts));
        counts[layer] = static_cast<size_t>(std::llround(value));
    }
    return counts;
}

std::vector<std::vector<size_t>> expert_placement(size_t num_experts,
                                                  size_t expert_parallel_degree) {
    if (expert_parallel_degree == 0) {
        throw std::invalid_argument("expert_placement: expert_parallel_degree must be > 0");
    }
    std::vector<std::vector<size_t>> placement(expert_parallel_degree);
    for (size_t expert = 0; expert < num_experts; ++expert) {
        placement[expert % expert_parallel_degree].push_back(expert);
    }
    return placement;
}

std::vector<DistillationStage> staged_distillation_schedule(size_t total_steps,
                                                            size_t stages,
                                                            double initial_kd_weight) {
    if (stages == 0) {
        throw std::invalid_argument("staged_distillation_schedule: stages must be > 0");
    }
    std::vector<DistillationStage> schedule;
    schedule.reserve(stages);

    const auto ranges = partition_evenly(total_steps, stages);
    for (size_t stage = 0; stage < stages; ++stage) {
        const double decay = 1.0 - static_cast<double>(stage) / static_cast<double>(stages);
        schedule.push_back(DistillationStage{
            ranges[stage].offset,
            ranges[stage].end(),
            initial_kd_weight * decay
        });
    }
    return schedule;
}

double distillation_loss(double student_task_loss,
                         double teacher_student_divergence,
                         const std::vector<DistillationStage>& schedule,
                         size_t step) {
    double kd_weight = 0.0;
    for (const auto& stage : schedule) {
        if (step >= stage.start_step && step < stage.end_step) {
            kd_weight = stage.kd_weight;
            break;
        }
    }
    return (1.0 - kd_weight) * student_task_loss + kd_weight * teacher_student_divergence;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
