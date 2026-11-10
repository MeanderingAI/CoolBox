#include "tyst_framework.hpp"

#include "decoder_language_model.h"
#include "tokenizer.h"
#include "tensor_backend.h"

#include <cmath>
#include <filesystem>

using namespace ml::deep_learning;
using namespace ml::deep_learning::llm;

TEST(ByteTokenizerTest, RoundTripsUtf8Bytes) {
    ByteTokenizer tokenizer;
    const std::string text = "CoolBox \xF0\x9F\x9A\x80";
    std::vector<std::size_t> encoded =
        tokenizer.encode(text, true, true);

    EXPECT_EQ(encoded.front(), 1u);
    EXPECT_EQ(encoded.back(), 2u);
    EXPECT_EQ(tokenizer.decode(encoded), text);
    EXPECT_EQ(
        tokenizer.vocabulary().size,
        ByteTokenizer::VOCAB_SIZE);
}

TEST(BpeTokenizerTest, TrainsRoundTripsAndPersists) {
    BpeTokenizer tokenizer;
    tokenizer.train(
        {"hello hello", "hello world", "hello hello"},
        280,
        2);
    const std::string text = "hello world";
    std::vector<std::size_t> encoded =
        tokenizer.encode(text, true, true);
    EXPECT_EQ(tokenizer.decode(encoded), text);
    EXPECT_GT(tokenizer.vocabulary().size, ByteTokenizer::VOCAB_SIZE);

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "coolbox_test_tokenizer.bpe";
    tokenizer.save(path.string());
    BpeTokenizer restored = BpeTokenizer::load(path.string());
    std::filesystem::remove(path);
    EXPECT_EQ(restored.encode(text), tokenizer.encode(text));
    EXPECT_EQ(restored.decode(encoded), text);
}

static DecoderLanguageModelConfig tiny_config() {
    DecoderLanguageModelConfig config;
    config.vocab_size = ByteTokenizer::VOCAB_SIZE;
    config.d_model = 8;
    config.num_heads = 2;
    config.d_ff = 16;
    config.num_layers = 1;
    config.max_sequence_length = 8;
    config.dropout_rate = 0.0;
    config.learning_rate = 0.01;
    config.weight_decay = 0.0;
    config.gradient_clip_norm = 1.0;
    config.seed = 7;
    return config;
}

TEST(DecoderLanguageModelTest, CausalPrefixIsStable) {
    set_tensor_backend(TensorBackend::CPU);
    DecoderLanguageModel model(tiny_config());
    Tensor prefix = model.logits({10, 11});
    Tensor extended = model.logits({10, 11, 12});

    const std::size_t vocabulary_size = tiny_config().vocab_size;
    for (std::size_t index = 0;
         index < 2 * vocabulary_size; ++index) {
        EXPECT_NEAR(prefix.data()[index], extended.data()[index], 1e-10);
    }
}

TEST(TensorBackendTest, ParallelCpuMatmulMatchesExpectedValues) {
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_cpu_threads(4);
    Tensor left({64, 256}, 1.5);
    Tensor right({256, 128}, 2.0);
    Tensor result = left.matmul(right);
    EXPECT_EQ(result.shape(), (std::vector<std::size_t>{64, 128}));
    for (double value : result.data()) {
        EXPECT_DOUBLE_EQ(value, 768.0);
    }
    set_tensor_cpu_threads(0);
}

TEST(TensorBackendTest, FusedVulkanOperationsMatchCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }

    Tensor input(
        {2, 4},
        {-1.0, 0.5, 2.0, -0.25, 3.0, -2.0, 0.75, 1.25});
    Tensor gradient(
        {2, 4},
        {0.5, -0.5, 1.0, 2.0, -1.0, 0.25, 0.75, -0.25});
    Tensor gamma({4}, {1.0, 0.5, 1.5, 2.0});
    Tensor beta({4}, {0.1, -0.2, 0.3, -0.4});
    Tensor weights(
        {4, 3},
        {0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
         0.7, 0.8, 0.9, 1.0, 1.1, 1.2});
    Tensor bias({3}, {0.25, -0.5, 0.75});
    Tensor mask(
        {2, 4},
        {1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0});
    Tensor first({2, 4}, 0.05);
    Tensor second({2, 4}, 0.01);

    set_tensor_backend(TensorBackend::CPU);
    const std::vector<Tensor> expected{
        input.matmul_bias(weights, bias),
        input.relu(),
        input.relu_backward(gradient),
        input.row_softmax(),
        input.masked_fill(mask, -1e9),
        input.layer_norm(gamma, beta, 1e-5),
        input.layer_norm_backward(gradient, gamma, 1e-5),
        input.layer_norm_gamma_gradient(gradient, 1e-5),
        input.row_sum(),
        input.broadcast_add(gamma),
        input.broadcast_multiply(gamma),
        input.slice_columns(1, 2),
        input.slice_columns(0, 2).concat_columns(
            input.slice_columns(2, 2)),
        input.adamw_update(first, second, 0.001, 0.1, 0.001, 1e-8),
    };
    const double expected_squared_sum = input.squared_sum();

    const std::vector<TensorPrecision> precisions{
        TensorPrecision::FP64,
        TensorPrecision::FP32,
        TensorPrecision::FP16,
        TensorPrecision::BF16,
    };
    for (TensorPrecision precision : precisions) {
        if (!tensor_precision_available(precision)) {
            continue;
        }
        set_tensor_precision(precision);
        set_tensor_backend(TensorBackend::VULKAN);
        const std::vector<Tensor> actual{
            input.matmul_bias(weights, bias),
            input.relu(),
            input.relu_backward(gradient),
            input.row_softmax(),
            input.masked_fill(mask, -1e9),
            input.layer_norm(gamma, beta, 1e-5),
            input.layer_norm_backward(gradient, gamma, 1e-5),
            input.layer_norm_gamma_gradient(gradient, 1e-5),
            input.row_sum(),
            input.broadcast_add(gamma),
            input.broadcast_multiply(gamma),
            input.slice_columns(1, 2),
            input.slice_columns(0, 2).concat_columns(
                input.slice_columns(2, 2)),
            input.adamw_update(
                first, second, 0.001, 0.1, 0.001, 1e-8),
        };
        const double tolerance =
            precision == TensorPrecision::FP64 ? 5e-8 :
            precision == TensorPrecision::FP32 ? 1e-5 :
            precision == TensorPrecision::FP16 ? 2e-2 : 5e-2;
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t tensor_index = 0;
             tensor_index < actual.size(); ++tensor_index) {
            ASSERT_EQ(
                actual[tensor_index].size(),
                expected[tensor_index].size());
            for (std::size_t value_index = 0;
                 value_index < actual[tensor_index].size();
                 ++value_index) {
                const double expected_value =
                    expected[tensor_index].data()[value_index];
                const double actual_value =
                    actual[tensor_index].data()[value_index];
                if (std::abs(expected_value) > 1e8) {
                    EXPECT_TRUE(actual_value < -1e8);
                } else {
                    EXPECT_NEAR(
                        actual_value, expected_value, tolerance);
                }
            }
        }
        EXPECT_NEAR(
            input.squared_sum(), expected_squared_sum,
            tolerance * expected_squared_sum);

        DecoderLanguageModel model(tiny_config());
        const double loss =
            model.train_step({1, 30, 31, 32, 2});
        EXPECT_TRUE(std::isfinite(loss));
        EXPECT_EQ(active_tensor_precision(), precision);
    }
    set_tensor_precision(TensorPrecision::AUTO);
    set_tensor_backend(TensorBackend::CPU);
}

TEST(DecoderLanguageModelTest, StateDictIsNamedAndStrictlyLoadable) {
    DecoderLanguageModel source(tiny_config());
    std::vector<NamedTensor> state = source.state_dict();
    ASSERT_EQ(state.size(), 19u);
    EXPECT_EQ(state.front().name, "model.embed_tokens.weight");
    EXPECT_EQ(state.back().name, "lm_head.bias");

    DecoderLanguageModel restored(tiny_config());
    restored.load_state_dict(state);
    Tensor source_logits = source.logits({10, 11});
    Tensor restored_logits = restored.logits({10, 11});
    ASSERT_EQ(source_logits.size(), restored_logits.size());
    for (std::size_t index = 0;
         index < source_logits.size();
         ++index) {
        EXPECT_DOUBLE_EQ(
            source_logits.data()[index],
            restored_logits.data()[index]);
    }

    state.pop_back();
    EXPECT_THROW(
        restored.load_state_dict(state),
        std::invalid_argument);
}

TEST(DecoderLanguageModelTest, TrainsGeneratesAndPersists) {
    DecoderLanguageModel model(tiny_config());
    const std::vector<std::size_t> sequence{1, 20, 21, 20, 21, 2};
    Tensor before = model.logits({1, 20, 21});
    const double loss = model.train_step(sequence);
    Tensor after = model.logits({1, 20, 21});

    EXPECT_TRUE(std::isfinite(loss));
    EXPECT_EQ(model.optimizer_step(), 1u);
    EXPECT_GT(model.parameter_count(), 0u);
    bool changed = false;
    for (std::size_t index = 0; index < before.size(); ++index) {
        if (std::abs(before.data()[index] - after.data()[index]) > 1e-12) {
            changed = true;
            break;
        }
    }
    EXPECT_TRUE(changed);

    GenerationConfig generation;
    generation.max_new_tokens = 3;
    generation.temperature = 0.0;
    std::vector<std::size_t> generated =
        model.generate({1, 20}, generation);
    EXPECT_EQ(generated.size(), 5u);

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "coolbox_test_decoder_lm.bin";
    model.save(path.string());
    std::unique_ptr<DecoderLanguageModel> restored =
        DecoderLanguageModel::load(path.string());
    std::filesystem::remove(path);
    Tensor restored_logits = restored->logits({1, 20, 21});
    for (std::size_t index = 0; index < after.size(); ++index) {
        EXPECT_NEAR(
            restored_logits.data()[index],
            after.data()[index],
            1e-12);
    }
    EXPECT_EQ(restored->optimizer_step(), model.optimizer_step());
}

TEST(DecoderLanguageModelTest, RunsOnSelectedVulkanBackend) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    set_tensor_backend(TensorBackend::VULKAN);
    DecoderLanguageModel model(tiny_config());
    const double loss = model.train_step({1, 30, 31, 32, 2});
    EXPECT_TRUE(std::isfinite(loss));
    EXPECT_EQ(active_tensor_backend(), TensorBackend::VULKAN);
    set_tensor_backend(TensorBackend::CPU);
}

int main(int argc, char** argv) {
    tyst::framework::init(&argc, argv);
    return tyst::framework::run_all_tests();
}
