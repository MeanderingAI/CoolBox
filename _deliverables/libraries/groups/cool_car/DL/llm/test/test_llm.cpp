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
