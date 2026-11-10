#ifndef COOLBOX_LLM_TOKENIZER_H
#define COOLBOX_LLM_TOKENIZER_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ml {
namespace deep_learning {
namespace llm {

struct VocabularyInfo {
    std::size_t size = 0;
    std::size_t pad_token_id = 0;
    std::size_t bos_token_id = 1;
    std::size_t eos_token_id = 2;
    std::size_t unk_token_id = 3;
};

class ByteTokenizer {
public:
    static constexpr std::size_t BYTE_TOKEN_OFFSET = 4;
    static constexpr std::size_t VOCAB_SIZE = 260;

    std::vector<std::size_t> encode(
        const std::string& text,
        bool add_bos = false,
        bool add_eos = false) const;
    std::string decode(
        const std::vector<std::size_t>& token_ids,
        bool skip_special_tokens = true) const;
    VocabularyInfo vocabulary() const;
};

class BpeTokenizer {
public:
    BpeTokenizer();

    void train(
        const std::vector<std::string>& corpus,
        std::size_t vocab_size,
        std::size_t min_frequency = 2,
        std::size_t thread_count = 0);
    std::vector<std::size_t> encode(
        const std::string& text,
        bool add_bos = false,
        bool add_eos = false) const;
    std::string decode(
        const std::vector<std::size_t>& token_ids,
        bool skip_special_tokens = true) const;

    VocabularyInfo vocabulary() const;
    const std::vector<std::pair<std::size_t, std::size_t>>& merges() const {
        return merges_;
    }

    void save(const std::string& path) const;
    static BpeTokenizer load(const std::string& path);

private:
    std::vector<std::pair<std::size_t, std::size_t>> merges_;
    std::vector<std::string> token_bytes_;

    void reset();
    void rebuild_token_bytes();
    static void apply_merge(
        std::vector<std::size_t>& tokens,
        std::size_t left,
        std::size_t right,
        std::size_t merged);
};

} // namespace llm
} // namespace deep_learning
} // namespace ml

#endif
