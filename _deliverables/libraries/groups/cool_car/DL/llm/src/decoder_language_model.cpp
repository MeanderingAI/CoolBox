#include "../include/decoder_language_model.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ml {
namespace deep_learning {
namespace llm {
namespace {

constexpr char MODEL_MAGIC[] = "CBXLLM1";
constexpr std::uint32_t MODEL_VERSION = 1;

std::vector<std::string> parameter_names(
    const DecoderLanguageModelConfig& config) {
    std::vector<std::string> names;
    names.push_back("model.embed_tokens.weight");
    for (std::size_t layer = 0;
         layer < config.num_layers;
         ++layer) {
        const std::string prefix =
            "model.layers." + std::to_string(layer) + ".";
        for (const std::string& projection :
             {"q_proj", "k_proj", "v_proj", "o_proj"}) {
            names.push_back(
                prefix + "self_attn." + projection + ".weight");
        }
        for (const std::string& projection :
             {"q_proj", "k_proj", "v_proj", "o_proj"}) {
            names.push_back(
                prefix + "self_attn." + projection + ".bias");
        }
        names.push_back(prefix + "mlp.fc1.weight");
        names.push_back(prefix + "mlp.fc1.bias");
        names.push_back(prefix + "mlp.fc2.weight");
        names.push_back(prefix + "mlp.fc2.bias");
        names.push_back(
            prefix + "input_layernorm.weight");
        names.push_back(
            prefix + "input_layernorm.bias");
        names.push_back(
            prefix + "post_attention_layernorm.weight");
        names.push_back(
            prefix + "post_attention_layernorm.bias");
    }
    names.push_back("lm_head.weight");
    names.push_back("lm_head.bias");
    return names;
}

template <typename T>
void write_value(std::ofstream& stream, const T& value) {
    stream.write(
        reinterpret_cast<const char*>(&value),
        static_cast<std::streamsize>(sizeof(T)));
    if (!stream) {
        throw std::runtime_error("Failed to write language model");
    }
}

template <typename T>
T read_value(std::ifstream& stream) {
    T value{};
    stream.read(
        reinterpret_cast<char*>(&value),
        static_cast<std::streamsize>(sizeof(T)));
    if (!stream) {
        throw std::runtime_error(
            "Invalid or truncated language model");
    }
    return value;
}

void write_tensor(std::ofstream& stream, const Tensor& tensor) {
    write_value(
        stream, static_cast<std::uint64_t>(tensor.ndim()));
    for (std::size_t dimension : tensor.shape()) {
        write_value(
            stream, static_cast<std::uint64_t>(dimension));
    }
    write_value(
        stream, static_cast<std::uint64_t>(tensor.size()));
    stream.write(
        reinterpret_cast<const char*>(tensor.data().data()),
        static_cast<std::streamsize>(
            tensor.size() * sizeof(double)));
    if (!stream) {
        throw std::runtime_error(
            "Failed to write language-model tensor");
    }
}

Tensor read_tensor(std::ifstream& stream) {
    const std::uint64_t rank = read_value<std::uint64_t>(stream);
    if (rank > 8) {
        throw std::runtime_error(
            "Language-model tensor rank is invalid");
    }
    std::vector<std::size_t> shape;
    shape.reserve(static_cast<std::size_t>(rank));
    for (std::uint64_t index = 0; index < rank; ++index) {
        shape.push_back(static_cast<std::size_t>(
            read_value<std::uint64_t>(stream)));
    }
    const std::uint64_t size = read_value<std::uint64_t>(stream);
    std::size_t expected_size = 1;
    for (std::size_t dimension : shape) {
        if (dimension != 0 &&
            expected_size >
                std::numeric_limits<std::size_t>::max() / dimension) {
            throw std::runtime_error(
                "Language-model tensor shape overflows");
        }
        expected_size *= dimension;
    }
    if (size != expected_size) {
        throw std::runtime_error(
            "Language-model tensor shape is inconsistent");
    }
    std::vector<double> data(static_cast<std::size_t>(size));
    stream.read(
        reinterpret_cast<char*>(data.data()),
        static_cast<std::streamsize>(
            data.size() * sizeof(double)));
    if (!stream) {
        throw std::runtime_error(
            "Language-model tensor data is truncated");
    }
    return Tensor(shape, data);
}

void validate_config(const DecoderLanguageModelConfig& config) {
    if (config.vocab_size < 2) {
        throw std::invalid_argument(
            "vocab_size must be at least 2");
    }
    if (config.d_model == 0 || config.num_heads == 0 ||
        config.d_model % config.num_heads != 0) {
        throw std::invalid_argument(
            "d_model must be positive and divisible by num_heads");
    }
    if (config.d_ff == 0 || config.num_layers == 0 ||
        config.max_sequence_length == 0) {
        throw std::invalid_argument(
            "d_ff, num_layers, and max_sequence_length must be positive");
    }
    if (config.dropout_rate < 0.0 ||
        config.dropout_rate >= 1.0) {
        throw std::invalid_argument(
            "dropout_rate must be in [0, 1)");
    }
    if (config.learning_rate <= 0.0 ||
        config.weight_decay < 0.0 ||
        config.beta1 < 0.0 || config.beta1 >= 1.0 ||
        config.beta2 < 0.0 || config.beta2 >= 1.0 ||
        config.epsilon <= 0.0 ||
        config.gradient_clip_norm < 0.0) {
        throw std::invalid_argument(
            "Optimizer configuration is invalid");
    }
}

} // namespace

DecoderLanguageModel::DecoderLanguageModel(
    const DecoderLanguageModelConfig& config)
    : config_(config),
      embedding_(config.vocab_size, config.d_model),
      positional_encoding_(
          config.d_model, config.max_sequence_length,
          config.dropout_rate),
      output_projection_(config.d_model, config.vocab_size) {
    validate_config(config_);
    blocks_.reserve(config_.num_layers);
    for (std::size_t index = 0; index < config_.num_layers; ++index) {
        blocks_.push_back(
            std::make_unique<DecoderOnlyTransformerLayer>(
                config_.d_model,
                config_.num_heads,
                config_.d_ff,
                config_.dropout_rate));
    }
}

void DecoderLanguageModel::validate_token_ids(
    const std::vector<std::size_t>& token_ids,
    bool allow_max_plus_one) const {
    if (token_ids.empty()) {
        throw std::invalid_argument(
            "Token sequence cannot be empty");
    }
    const std::size_t maximum =
        config_.max_sequence_length + (allow_max_plus_one ? 1 : 0);
    if (token_ids.size() > maximum) {
        throw std::invalid_argument(
            "Token sequence exceeds max_sequence_length");
    }
    for (std::size_t token : token_ids) {
        if (token >= config_.vocab_size) {
            throw std::out_of_range(
                "Token ID exceeds language-model vocabulary");
        }
    }
}

void DecoderLanguageModel::set_training(bool training) {
    training_ = training;
    positional_encoding_.set_training(training);
    for (auto& block : blocks_) {
        block->set_training(training);
    }
}

Tensor DecoderLanguageModel::forward(
    const std::vector<std::size_t>& input_ids) {
    validate_token_ids(input_ids);
    std::vector<double> token_values;
    token_values.reserve(input_ids.size());
    for (std::size_t token : input_ids) {
        token_values.push_back(static_cast<double>(token));
    }

    Tensor token_tensor({1, input_ids.size()}, token_values);
    Tensor hidden = embedding_.forward(token_tensor).reshape(
        {input_ids.size(), config_.d_model});
    hidden = positional_encoding_.forward(hidden);
    for (auto& block : blocks_) {
        hidden = block->forward(hidden);
    }
    return output_projection_.forward(hidden);
}

Tensor DecoderLanguageModel::logits(
    const std::vector<std::size_t>& input_ids) {
    set_training(false);
    return forward(input_ids);
}

std::pair<double, Tensor>
DecoderLanguageModel::cross_entropy_with_logits(
    const Tensor& logits,
    const std::vector<std::size_t>& target_ids) {
    if (logits.ndim() != 2 ||
        logits.shape()[0] != target_ids.size()) {
        throw std::invalid_argument(
            "Logit rows must match target token count");
    }
    const std::size_t rows = logits.shape()[0];
    const std::size_t vocabulary_size = logits.shape()[1];
    Tensor gradient(logits.shape(), 0.0);
    double loss = 0.0;

    for (std::size_t row = 0; row < rows; ++row) {
        if (target_ids[row] >= vocabulary_size) {
            throw std::out_of_range(
                "Target token exceeds logit vocabulary");
        }
        double maximum = -std::numeric_limits<double>::infinity();
        for (std::size_t token = 0; token < vocabulary_size; ++token) {
            maximum = std::max(
                maximum,
                logits.data()[row * vocabulary_size + token]);
        }
        double denominator = 0.0;
        for (std::size_t token = 0; token < vocabulary_size; ++token) {
            const double probability = std::exp(
                logits.data()[row * vocabulary_size + token] - maximum);
            gradient.data()[row * vocabulary_size + token] = probability;
            denominator += probability;
        }
        const double target_probability = std::max(
            gradient.data()[
                row * vocabulary_size + target_ids[row]] / denominator,
            std::numeric_limits<double>::min());
        loss -= std::log(target_probability);
        for (std::size_t token = 0; token < vocabulary_size; ++token) {
            gradient.data()[row * vocabulary_size + token] /= denominator;
        }
        gradient.data()[
            row * vocabulary_size + target_ids[row]] -= 1.0;
    }

    const double scale = 1.0 / static_cast<double>(rows);
    for (double& value : gradient.data()) {
        value *= scale;
    }
    return {loss * scale, std::move(gradient)};
}

std::vector<TrainableParameter>
DecoderLanguageModel::trainable_parameters() {
    std::vector<TrainableParameter> parameters;
    embedding_.collect_trainable_parameters(parameters);
    for (auto& block : blocks_) {
        block->collect_trainable_parameters(parameters);
    }
    output_projection_.collect_trainable_parameters(parameters);
    return parameters;
}

void DecoderLanguageModel::apply_optimizer() {
    std::vector<TrainableParameter> parameters =
        trainable_parameters();
    Tensor gradient_norm_squared_tensor;
    bool first_gradient = true;
    for (const TrainableParameter& parameter : parameters) {
        Tensor parameter_norm =
            parameter.gradients->squared_sum_tensor();
        gradient_norm_squared_tensor = first_gradient
            ? std::move(parameter_norm)
            : gradient_norm_squared_tensor + parameter_norm;
        first_gradient = false;
    }
    const double gradient_norm_squared =
        first_gradient ? 0.0 : gradient_norm_squared_tensor.data()[0];
    const double gradient_norm = std::sqrt(gradient_norm_squared);
    double gradient_scale = 1.0;
    if (config_.gradient_clip_norm > 0.0 &&
        gradient_norm > config_.gradient_clip_norm) {
        gradient_scale =
            config_.gradient_clip_norm / gradient_norm;
    }

    ++optimizer_step_;
    for (TrainableParameter& parameter : parameters) {
        Tensor& values = *parameter.values;
        Tensor& gradients = *parameter.gradients;
        if (values.shape() != gradients.shape()) {
            throw std::logic_error(
                "Parameter and gradient shapes differ");
        }

        if (config_.optimizer == LanguageModelOptimizer::SGD) {
            if (tensor_backend() != TensorBackend::CPU) {
                Tensor update = gradients * gradient_scale;
                if (parameter.apply_weight_decay) {
                    update = update +
                        values * config_.weight_decay;
                }
                values = values - update * config_.learning_rate;
                continue;
            }
            for (std::size_t index = 0; index < values.size(); ++index) {
                const double gradient =
                    gradients.data()[index] * gradient_scale;
                const double decay =
                    parameter.apply_weight_decay
                        ? config_.weight_decay * values.data()[index]
                        : 0.0;
                values.data()[index] -=
                    config_.learning_rate * (gradient + decay);
            }
            continue;
        }

        AdamState& state = adam_states_[&values];
        if (state.first_moment.shape() != values.shape()) {
            state.first_moment = Tensor(values.shape(), 0.0);
            state.second_moment = Tensor(values.shape(), 0.0);
        }
        const double beta1_correction =
            1.0 - std::pow(
                config_.beta1,
                static_cast<double>(optimizer_step_));
        const double beta2_correction =
            1.0 - std::pow(
                config_.beta2,
                static_cast<double>(optimizer_step_));
        if (tensor_backend() != TensorBackend::CPU) {
            Tensor scaled_gradient = gradients * gradient_scale;
            state.first_moment =
                state.first_moment * config_.beta1 +
                scaled_gradient * (1.0 - config_.beta1);
            state.second_moment =
                state.second_moment * config_.beta2 +
                (scaled_gradient * scaled_gradient) *
                    (1.0 - config_.beta2);
            values = values.adamw_update(
                state.first_moment,
                state.second_moment,
                config_.learning_rate,
                beta1_correction,
                beta2_correction,
                config_.epsilon);
            if (parameter.apply_weight_decay) {
                values = values * (
                    1.0 - config_.learning_rate *
                    config_.weight_decay);
            }
            continue;
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            const double gradient =
                gradients.data()[index] * gradient_scale;
            double& first = state.first_moment.data()[index];
            double& second = state.second_moment.data()[index];
            first =
                config_.beta1 * first +
                (1.0 - config_.beta1) * gradient;
            second =
                config_.beta2 * second +
                (1.0 - config_.beta2) * gradient * gradient;
            const double update =
                (first / beta1_correction) /
                (std::sqrt(second / beta2_correction) +
                 config_.epsilon);
            values.data()[index] -=
                config_.learning_rate * update;
            if (parameter.apply_weight_decay) {
                values.data()[index] -=
                    config_.learning_rate *
                    config_.weight_decay *
                    values.data()[index];
            }
        }
    }
}

double DecoderLanguageModel::train_step(
    const std::vector<std::size_t>& token_ids) {
    validate_token_ids(token_ids, true);
    if (token_ids.size() < 2) {
        throw std::invalid_argument(
            "Training requires at least two tokens");
    }
    std::vector<std::size_t> input_ids(
        token_ids.begin(), token_ids.end() - 1);
    std::vector<std::size_t> target_ids(
        token_ids.begin() + 1, token_ids.end());

    set_training(true);
    Tensor predictions = forward(input_ids);
    auto loss_and_gradient =
        cross_entropy_with_logits(predictions, target_ids);
    Tensor gradient =
        output_projection_.backward(loss_and_gradient.second);
    for (auto block = blocks_.rbegin();
         block != blocks_.rend(); ++block) {
        gradient = (*block)->backward(gradient);
    }
    gradient = positional_encoding_.backward(gradient);
    embedding_.backward(
        gradient.reshape(
            {1, input_ids.size(), config_.d_model}));
    apply_optimizer();
    return loss_and_gradient.first;
}

std::vector<std::vector<std::size_t>>
DecoderLanguageModel::split_training_sequence(
    const std::vector<std::size_t>& tokens) const {
    std::vector<std::vector<std::size_t>> chunks;
    if (tokens.size() < 2) {
        return chunks;
    }
    const std::size_t chunk_size =
        config_.max_sequence_length + 1;
    for (std::size_t start = 0; start + 1 < tokens.size();) {
        const std::size_t end =
            std::min(tokens.size(), start + chunk_size);
        chunks.emplace_back(
            tokens.begin() + static_cast<std::ptrdiff_t>(start),
            tokens.begin() + static_cast<std::ptrdiff_t>(end));
        if (end == tokens.size()) {
            break;
        }
        start += config_.max_sequence_length;
    }
    return chunks;
}

std::vector<double> DecoderLanguageModel::fit(
    const std::vector<std::vector<std::size_t>>& token_sequences,
    std::size_t epochs,
    bool verbose) {
    if (token_sequences.empty() || epochs == 0) {
        throw std::invalid_argument(
            "fit requires sequences and at least one epoch");
    }
    std::vector<double> epoch_losses;
    epoch_losses.reserve(epochs);
    for (std::size_t epoch = 0; epoch < epochs; ++epoch) {
        double total_loss = 0.0;
        std::size_t steps = 0;
        for (const auto& sequence : token_sequences) {
            for (const auto& chunk : split_training_sequence(sequence)) {
                total_loss += train_step(chunk);
                ++steps;
            }
        }
        if (steps == 0) {
            throw std::invalid_argument(
                "fit requires a sequence with at least two tokens");
        }
        const double average =
            total_loss / static_cast<double>(steps);
        epoch_losses.push_back(average);
        if (verbose) {
            std::cout << "Epoch " << (epoch + 1) << "/" << epochs
                      << " - loss: " << average << '\n';
        }
    }
    return epoch_losses;
}

double DecoderLanguageModel::evaluate(
    const std::vector<std::vector<std::size_t>>& token_sequences) {
    if (token_sequences.empty()) {
        throw std::invalid_argument(
            "evaluate requires token sequences");
    }
    set_training(false);
    double total_loss = 0.0;
    std::size_t steps = 0;
    for (const auto& sequence : token_sequences) {
        for (const auto& chunk : split_training_sequence(sequence)) {
            std::vector<std::size_t> input(
                chunk.begin(), chunk.end() - 1);
            std::vector<std::size_t> target(
                chunk.begin() + 1, chunk.end());
            total_loss +=
                cross_entropy_with_logits(forward(input), target).first;
            ++steps;
        }
    }
    if (steps == 0) {
        throw std::invalid_argument(
            "evaluate requires a sequence with at least two tokens");
    }
    return total_loss / static_cast<double>(steps);
}

std::vector<std::size_t> DecoderLanguageModel::generate(
    const std::vector<std::size_t>& prompt,
    const GenerationConfig& generation) {
    if (prompt.empty()) {
        throw std::invalid_argument(
            "Generation prompt cannot be empty");
    }
    if (generation.temperature < 0.0 ||
        generation.top_p <= 0.0 || generation.top_p > 1.0) {
        throw std::invalid_argument(
            "temperature must be nonnegative and top_p must be in (0, 1]");
    }
    for (std::size_t token : prompt) {
        if (token >= config_.vocab_size) {
            throw std::out_of_range(
                "Prompt token exceeds vocabulary");
        }
    }

    std::uint64_t seed = generation.seed;
    if (seed == 0) {
        seed = config_.seed;
    }
    if (seed == 0) {
        seed = std::random_device{}();
    }
    std::mt19937_64 generator(seed);
    std::vector<std::size_t> result = prompt;

    for (std::size_t generated = 0;
         generated < generation.max_new_tokens; ++generated) {
        const std::size_t context_start =
            result.size() > config_.max_sequence_length
                ? result.size() - config_.max_sequence_length
                : 0;
        std::vector<std::size_t> context(
            result.begin() +
                static_cast<std::ptrdiff_t>(context_start),
            result.end());
        Tensor output = logits(context);
        const std::size_t row = output.shape()[0] - 1;
        const std::size_t vocabulary_size = output.shape()[1];
        const double* row_logits =
            output.data().data() + row * vocabulary_size;

        std::size_t next_token = 0;
        if (generation.temperature == 0.0) {
            next_token = static_cast<std::size_t>(
                std::max_element(
                    row_logits, row_logits + vocabulary_size) -
                row_logits);
        } else {
            std::vector<std::pair<std::size_t, double>> candidates;
            candidates.reserve(vocabulary_size);
            const double maximum = *std::max_element(
                row_logits, row_logits + vocabulary_size);
            for (std::size_t token = 0;
                 token < vocabulary_size; ++token) {
                candidates.push_back({
                    token,
                    std::exp(
                        (row_logits[token] - maximum) /
                        generation.temperature)});
            }
            std::sort(
                candidates.begin(), candidates.end(),
                [](const auto& left, const auto& right) {
                    return left.second > right.second;
                });
            if (generation.top_k > 0 &&
                generation.top_k < candidates.size()) {
                candidates.resize(generation.top_k);
            }
            if (generation.top_p < 1.0) {
                const double total = std::accumulate(
                    candidates.begin(), candidates.end(), 0.0,
                    [](double sum, const auto& candidate) {
                        return sum + candidate.second;
                    });
                double cumulative = 0.0;
                std::size_t keep = 0;
                while (keep < candidates.size()) {
                    cumulative += candidates[keep].second / total;
                    ++keep;
                    if (cumulative >= generation.top_p) {
                        break;
                    }
                }
                candidates.resize(std::max<std::size_t>(1, keep));
            }
            std::vector<double> weights;
            weights.reserve(candidates.size());
            for (const auto& candidate : candidates) {
                weights.push_back(candidate.second);
            }
            std::discrete_distribution<std::size_t> distribution(
                weights.begin(), weights.end());
            next_token = candidates[distribution(generator)].first;
        }

        result.push_back(next_token);
        if (generation.eos_token_id.has_value() &&
            next_token == generation.eos_token_id.value()) {
            break;
        }
    }
    return result;
}

std::size_t DecoderLanguageModel::parameter_count() {
    std::size_t count = 0;
    for (const TrainableParameter& parameter :
         trainable_parameters()) {
        count += parameter.values->size();
    }
    return count;
}

std::vector<NamedTensor> DecoderLanguageModel::state_dict() {
    const std::vector<TrainableParameter> parameters =
        trainable_parameters();
    const std::vector<std::string> names =
        parameter_names(config_);
    if (names.size() != parameters.size()) {
        throw std::runtime_error(
            "Language-model parameter naming is inconsistent");
    }
    std::vector<NamedTensor> result;
    result.reserve(parameters.size());
    for (std::size_t index = 0;
         index < parameters.size();
         ++index) {
        result.push_back(
            NamedTensor{names[index], parameters[index].values->clone()});
    }
    return result;
}

void DecoderLanguageModel::load_state_dict(
    const std::vector<NamedTensor>& tensors,
    bool strict) {
    std::vector<TrainableParameter> parameters =
        trainable_parameters();
    const std::vector<std::string> names =
        parameter_names(config_);
    if (names.size() != parameters.size()) {
        throw std::runtime_error(
            "Language-model parameter naming is inconsistent");
    }
    std::unordered_map<std::string, const Tensor*> supplied;
    for (const NamedTensor& tensor : tensors) {
        if (!supplied.emplace(tensor.name, &tensor.tensor).second) {
            throw std::invalid_argument(
                "Duplicate state-dict tensor: " + tensor.name);
        }
    }
    for (std::size_t index = 0;
         index < names.size();
         ++index) {
        const auto tensor = supplied.find(names[index]);
        if (tensor == supplied.end()) {
            if (strict) {
                throw std::invalid_argument(
                    "Missing state-dict tensor: " + names[index]);
            }
            continue;
        }
        if (tensor->second->shape() !=
            parameters[index].values->shape()) {
            throw std::invalid_argument(
                "State-dict tensor shape mismatch: " + names[index]);
        }
        *parameters[index].values = tensor->second->clone();
        supplied.erase(tensor);
    }
    if (strict && !supplied.empty()) {
        throw std::invalid_argument(
            "Unexpected state-dict tensor: " +
            supplied.begin()->first);
    }
    adam_states_.clear();
    optimizer_step_ = 0;
}

void DecoderLanguageModel::save(const std::string& path) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(
            "Unable to open language model for writing: " + path);
    }
    stream.write(MODEL_MAGIC, sizeof(MODEL_MAGIC));
    write_value(stream, MODEL_VERSION);
    write_value(stream, static_cast<std::uint64_t>(config_.vocab_size));
    write_value(stream, static_cast<std::uint64_t>(config_.d_model));
    write_value(stream, static_cast<std::uint64_t>(config_.num_heads));
    write_value(stream, static_cast<std::uint64_t>(config_.d_ff));
    write_value(stream, static_cast<std::uint64_t>(config_.num_layers));
    write_value(
        stream,
        static_cast<std::uint64_t>(config_.max_sequence_length));
    write_value(stream, config_.dropout_rate);
    write_value(
        stream, static_cast<std::uint32_t>(config_.optimizer));
    write_value(stream, config_.learning_rate);
    write_value(stream, config_.weight_decay);
    write_value(stream, config_.beta1);
    write_value(stream, config_.beta2);
    write_value(stream, config_.epsilon);
    write_value(stream, config_.gradient_clip_norm);
    write_value(stream, config_.seed);

    std::vector<TrainableParameter> parameters =
        trainable_parameters();
    write_value(
        stream, static_cast<std::uint64_t>(parameters.size()));
    for (const TrainableParameter& parameter : parameters) {
        write_tensor(stream, *parameter.values);
    }
    write_value(
        stream, static_cast<std::uint64_t>(optimizer_step_));
    for (const TrainableParameter& parameter : parameters) {
        const auto state = adam_states_.find(parameter.values);
        const std::uint8_t has_state =
            state == adam_states_.end() ? 0 : 1;
        write_value(stream, has_state);
        if (has_state != 0) {
            write_tensor(stream, state->second.first_moment);
            write_tensor(stream, state->second.second_moment);
        }
    }
}

std::unique_ptr<DecoderLanguageModel>
DecoderLanguageModel::load(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(
            "Unable to open language model: " + path);
    }
    char magic[sizeof(MODEL_MAGIC)]{};
    stream.read(magic, sizeof(magic));
    if (!stream ||
        std::string(magic, sizeof(magic)) !=
            std::string(MODEL_MAGIC, sizeof(MODEL_MAGIC))) {
        throw std::runtime_error("Invalid language model file");
    }
    if (read_value<std::uint32_t>(stream) != MODEL_VERSION) {
        throw std::runtime_error(
            "Unsupported language model version");
    }

    DecoderLanguageModelConfig config;
    config.vocab_size = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.d_model = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.num_heads = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.d_ff = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.num_layers = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.max_sequence_length = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    config.dropout_rate = read_value<double>(stream);
    config.optimizer = static_cast<LanguageModelOptimizer>(
        read_value<std::uint32_t>(stream));
    config.learning_rate = read_value<double>(stream);
    config.weight_decay = read_value<double>(stream);
    config.beta1 = read_value<double>(stream);
    config.beta2 = read_value<double>(stream);
    config.epsilon = read_value<double>(stream);
    config.gradient_clip_norm = read_value<double>(stream);
    config.seed = read_value<std::uint64_t>(stream);
    if (config.optimizer != LanguageModelOptimizer::SGD &&
        config.optimizer != LanguageModelOptimizer::ADAMW) {
        throw std::runtime_error(
            "Language model contains an invalid optimizer");
    }

    auto model =
        std::make_unique<DecoderLanguageModel>(config);
    std::vector<TrainableParameter> parameters =
        model->trainable_parameters();
    const std::uint64_t parameter_count =
        read_value<std::uint64_t>(stream);
    if (parameter_count != parameters.size()) {
        throw std::runtime_error(
            "Language-model parameter count does not match");
    }
    for (TrainableParameter& parameter : parameters) {
        Tensor loaded = read_tensor(stream);
        if (loaded.shape() != parameter.values->shape()) {
            throw std::runtime_error(
                "Language-model parameter shape does not match");
        }
        *parameter.values = std::move(loaded);
    }

    model->optimizer_step_ = static_cast<std::size_t>(
        read_value<std::uint64_t>(stream));
    for (TrainableParameter& parameter : parameters) {
        const std::uint8_t has_state =
            read_value<std::uint8_t>(stream);
        if (has_state == 0) {
            continue;
        }
        AdamState state{
            read_tensor(stream),
            read_tensor(stream),
        };
        if (state.first_moment.shape() != parameter.values->shape() ||
            state.second_moment.shape() != parameter.values->shape()) {
            throw std::runtime_error(
                "Language-model optimizer state shape does not match");
        }
        model->adam_states_.emplace(
            parameter.values, std::move(state));
    }
    return model;
}

} // namespace llm
} // namespace deep_learning
} // namespace ml
