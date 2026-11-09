#ifndef COOLBOX_DECODER_LANGUAGE_MODEL_H
#define COOLBOX_DECODER_LANGUAGE_MODEL_H

#include "../../layers/include/layer.h"
#include "../../layers/include/tensor.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ml {
namespace deep_learning {
namespace llm {

enum class LanguageModelOptimizer {
    SGD,
    ADAMW
};

struct DecoderLanguageModelConfig {
    std::size_t vocab_size = 260;
    std::size_t d_model = 128;
    std::size_t num_heads = 4;
    std::size_t d_ff = 512;
    std::size_t num_layers = 4;
    std::size_t max_sequence_length = 256;
    double dropout_rate = 0.1;
    LanguageModelOptimizer optimizer = LanguageModelOptimizer::ADAMW;
    double learning_rate = 0.001;
    double weight_decay = 0.01;
    double beta1 = 0.9;
    double beta2 = 0.999;
    double epsilon = 1e-8;
    double gradient_clip_norm = 1.0;
    std::uint64_t seed = 0;
};

struct GenerationConfig {
    std::size_t max_new_tokens = 32;
    double temperature = 1.0;
    std::size_t top_k = 0;
    double top_p = 1.0;
    std::optional<std::size_t> eos_token_id;
    std::uint64_t seed = 0;
};

struct NamedTensor {
    std::string name;
    Tensor tensor;
};

class DecoderLanguageModel {
public:
    explicit DecoderLanguageModel(
        const DecoderLanguageModelConfig& config);

    Tensor logits(const std::vector<std::size_t>& input_ids);
    double train_step(const std::vector<std::size_t>& token_ids);
    std::vector<double> fit(
        const std::vector<std::vector<std::size_t>>& token_sequences,
        std::size_t epochs = 1,
        bool verbose = false);
    double evaluate(
        const std::vector<std::vector<std::size_t>>& token_sequences);
    std::vector<std::size_t> generate(
        const std::vector<std::size_t>& prompt,
        const GenerationConfig& generation = {});

    void set_training(bool training);
    bool training() const { return training_; }
    const DecoderLanguageModelConfig& config() const { return config_; }
    std::size_t parameter_count();
    std::size_t optimizer_step() const { return optimizer_step_; }
    std::vector<NamedTensor> state_dict();
    void load_state_dict(
        const std::vector<NamedTensor>& tensors,
        bool strict = true);

    void save(const std::string& path);
    static std::unique_ptr<DecoderLanguageModel> load(
        const std::string& path);

private:
    struct AdamState {
        Tensor first_moment;
        Tensor second_moment;
    };

    DecoderLanguageModelConfig config_;
    EmbeddingLayer embedding_;
    PositionalEncodingLayer positional_encoding_;
    std::vector<std::unique_ptr<DecoderOnlyTransformerLayer>> blocks_;
    DenseLayer output_projection_;
    bool training_ = true;
    std::size_t optimizer_step_ = 0;
    std::unordered_map<Tensor*, AdamState> adam_states_;

    Tensor forward(const std::vector<std::size_t>& input_ids);
    static std::pair<double, Tensor> cross_entropy_with_logits(
        const Tensor& logits,
        const std::vector<std::size_t>& target_ids);
    std::vector<TrainableParameter> trainable_parameters();
    void apply_optimizer();
    std::vector<std::vector<std::size_t>> split_training_sequence(
        const std::vector<std::size_t>& tokens) const;
    void validate_token_ids(
        const std::vector<std::size_t>& token_ids,
        bool allow_max_plus_one = false) const;
};

} // namespace llm
} // namespace deep_learning
} // namespace ml

#endif
