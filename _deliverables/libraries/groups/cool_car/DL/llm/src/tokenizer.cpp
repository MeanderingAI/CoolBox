#include "../include/tokenizer.h"

#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace llm {
namespace {

constexpr std::uint32_t BPE_FILE_VERSION = 1;
constexpr char BPE_MAGIC[] = "CBXBPE1";

std::vector<std::size_t> bytes_to_tokens(const std::string& text) {
    std::vector<std::size_t> tokens;
    tokens.reserve(text.size());
    for (unsigned char byte : text) {
        tokens.push_back(ByteTokenizer::BYTE_TOKEN_OFFSET + byte);
    }
    return tokens;
}

template <typename T>
void write_value(std::ofstream& stream, const T& value) {
    stream.write(
        reinterpret_cast<const char*>(&value),
        static_cast<std::streamsize>(sizeof(T)));
    if (!stream) {
        throw std::runtime_error("Failed to write BPE tokenizer");
    }
}

template <typename T>
T read_value(std::ifstream& stream) {
    T value{};
    stream.read(
        reinterpret_cast<char*>(&value),
        static_cast<std::streamsize>(sizeof(T)));
    if (!stream) {
        throw std::runtime_error("Invalid or truncated BPE tokenizer");
    }
    return value;
}

} // namespace

std::vector<std::size_t> ByteTokenizer::encode(
    const std::string& text,
    bool add_bos,
    bool add_eos) const {
    std::vector<std::size_t> tokens;
    tokens.reserve(text.size() + (add_bos ? 1 : 0) + (add_eos ? 1 : 0));
    if (add_bos) {
        tokens.push_back(1);
    }
    std::vector<std::size_t> bytes = bytes_to_tokens(text);
    tokens.insert(tokens.end(), bytes.begin(), bytes.end());
    if (add_eos) {
        tokens.push_back(2);
    }
    return tokens;
}

std::string ByteTokenizer::decode(
    const std::vector<std::size_t>& token_ids,
    bool skip_special_tokens) const {
    std::string result;
    result.reserve(token_ids.size());
    for (std::size_t token : token_ids) {
        if (token < BYTE_TOKEN_OFFSET) {
            if (skip_special_tokens) {
                continue;
            }
            if (token == 3) {
                result += "<unk>";
            }
            continue;
        }
        if (token >= VOCAB_SIZE) {
            if (!skip_special_tokens) {
                result += "<unk>";
            }
            continue;
        }
        result.push_back(
            static_cast<char>(token - BYTE_TOKEN_OFFSET));
    }
    return result;
}

VocabularyInfo ByteTokenizer::vocabulary() const {
    return {VOCAB_SIZE, 0, 1, 2, 3};
}

BpeTokenizer::BpeTokenizer() {
    reset();
}

void BpeTokenizer::reset() {
    merges_.clear();
    token_bytes_.assign(ByteTokenizer::VOCAB_SIZE, {});
    for (std::size_t byte = 0; byte < 256; ++byte) {
        token_bytes_[ByteTokenizer::BYTE_TOKEN_OFFSET + byte] =
            std::string(1, static_cast<char>(byte));
    }
}

void BpeTokenizer::apply_merge(
    std::vector<std::size_t>& tokens,
    std::size_t left,
    std::size_t right,
    std::size_t merged) {
    if (tokens.size() < 2) {
        return;
    }
    std::vector<std::size_t> result;
    result.reserve(tokens.size());
    for (std::size_t index = 0; index < tokens.size();) {
        if (index + 1 < tokens.size() &&
            tokens[index] == left &&
            tokens[index + 1] == right) {
            result.push_back(merged);
            index += 2;
        } else {
            result.push_back(tokens[index]);
            ++index;
        }
    }
    tokens.swap(result);
}

void BpeTokenizer::train(
    const std::vector<std::string>& corpus,
    std::size_t vocab_size,
    std::size_t min_frequency) {
    if (corpus.empty()) {
        throw std::invalid_argument("BPE corpus cannot be empty");
    }
    if (vocab_size < ByteTokenizer::VOCAB_SIZE) {
        throw std::invalid_argument(
            "BPE vocabulary size must be at least 260");
    }
    if (min_frequency == 0) {
        throw std::invalid_argument(
            "BPE minimum frequency must be positive");
    }

    reset();
    std::vector<std::vector<std::size_t>> sequences;
    sequences.reserve(corpus.size());
    for (const std::string& text : corpus) {
        sequences.push_back(bytes_to_tokens(text));
    }

    while (token_bytes_.size() < vocab_size) {
        std::map<std::pair<std::size_t, std::size_t>, std::size_t>
            frequencies;
        for (const auto& sequence : sequences) {
            for (std::size_t index = 1; index < sequence.size(); ++index) {
                ++frequencies[
                    {sequence[index - 1], sequence[index]}];
            }
        }

        std::pair<std::size_t, std::size_t> best_pair{};
        std::size_t best_frequency = 0;
        for (const auto& entry : frequencies) {
            if (entry.second > best_frequency) {
                best_pair = entry.first;
                best_frequency = entry.second;
            }
        }
        if (best_frequency < min_frequency) {
            break;
        }

        const std::size_t merged_id = token_bytes_.size();
        merges_.push_back(best_pair);
        token_bytes_.push_back(
            token_bytes_.at(best_pair.first) +
            token_bytes_.at(best_pair.second));
        for (auto& sequence : sequences) {
            apply_merge(
                sequence, best_pair.first, best_pair.second, merged_id);
        }
    }
}

std::vector<std::size_t> BpeTokenizer::encode(
    const std::string& text,
    bool add_bos,
    bool add_eos) const {
    std::vector<std::size_t> tokens = bytes_to_tokens(text);
    for (std::size_t index = 0; index < merges_.size(); ++index) {
        apply_merge(
            tokens, merges_[index].first, merges_[index].second,
            ByteTokenizer::VOCAB_SIZE + index);
    }
    if (add_bos) {
        tokens.insert(tokens.begin(), 1);
    }
    if (add_eos) {
        tokens.push_back(2);
    }
    return tokens;
}

std::string BpeTokenizer::decode(
    const std::vector<std::size_t>& token_ids,
    bool skip_special_tokens) const {
    std::string result;
    for (std::size_t token : token_ids) {
        if (token < ByteTokenizer::BYTE_TOKEN_OFFSET) {
            if (!skip_special_tokens && token == 3) {
                result += "<unk>";
            }
            continue;
        }
        if (token >= token_bytes_.size()) {
            if (!skip_special_tokens) {
                result += "<unk>";
            }
            continue;
        }
        result += token_bytes_[token];
    }
    return result;
}

VocabularyInfo BpeTokenizer::vocabulary() const {
    return {token_bytes_.size(), 0, 1, 2, 3};
}

void BpeTokenizer::save(const std::string& path) const {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(
            "Unable to open BPE tokenizer for writing: " + path);
    }
    stream.write(BPE_MAGIC, sizeof(BPE_MAGIC));
    write_value(stream, BPE_FILE_VERSION);
    write_value(
        stream, static_cast<std::uint64_t>(merges_.size()));
    for (const auto& merge : merges_) {
        write_value(
            stream, static_cast<std::uint64_t>(merge.first));
        write_value(
            stream, static_cast<std::uint64_t>(merge.second));
    }
}

BpeTokenizer BpeTokenizer::load(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(
            "Unable to open BPE tokenizer: " + path);
    }
    char magic[sizeof(BPE_MAGIC)]{};
    stream.read(magic, sizeof(magic));
    if (!stream ||
        std::string(magic, sizeof(magic)) !=
            std::string(BPE_MAGIC, sizeof(BPE_MAGIC))) {
        throw std::runtime_error("Invalid BPE tokenizer file");
    }
    if (read_value<std::uint32_t>(stream) != BPE_FILE_VERSION) {
        throw std::runtime_error(
            "Unsupported BPE tokenizer version");
    }

    BpeTokenizer tokenizer;
    const std::uint64_t merge_count =
        read_value<std::uint64_t>(stream);
    tokenizer.merges_.reserve(
        static_cast<std::size_t>(merge_count));
    for (std::uint64_t index = 0; index < merge_count; ++index) {
        const std::size_t left = static_cast<std::size_t>(
            read_value<std::uint64_t>(stream));
        const std::size_t right = static_cast<std::size_t>(
            read_value<std::uint64_t>(stream));
        tokenizer.merges_.push_back({left, right});
    }
    tokenizer.rebuild_token_bytes();
    return tokenizer;
}

void BpeTokenizer::rebuild_token_bytes() {
    token_bytes_.assign(ByteTokenizer::VOCAB_SIZE, {});
    for (std::size_t byte = 0; byte < 256; ++byte) {
        token_bytes_[ByteTokenizer::BYTE_TOKEN_OFFSET + byte] =
            std::string(1, static_cast<char>(byte));
    }
    for (const auto& merge : merges_) {
        if (merge.first >= token_bytes_.size() ||
            merge.second >= token_bytes_.size()) {
            throw std::runtime_error(
                "BPE tokenizer contains an invalid merge");
        }
        token_bytes_.push_back(
            token_bytes_[merge.first] + token_bytes_[merge.second]);
    }
}

} // namespace llm
} // namespace deep_learning
} // namespace ml
