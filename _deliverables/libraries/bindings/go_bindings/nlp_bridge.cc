#include "nlp/text_processor.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {

struct StringVector {
    std::vector<std::string> values;
};

char* copy_string(const std::string& value) {
    auto* copy = static_cast<char*>(std::malloc(value.size() + 1));
    if (copy == nullptr) {
        return nullptr;
    }
    std::memcpy(copy, value.data(), value.size());
    copy[value.size()] = '\0';
    return copy;
}

} // namespace

extern "C" {

char* coolbox_nlp_lowercase(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    try {
        return copy_string(ml::nlp::TextProcessor::to_lowercase(text));
    } catch (...) {
        return nullptr;
    }
}

char* coolbox_nlp_remove_punctuation(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    try {
        return copy_string(ml::nlp::TextProcessor::remove_punctuation(text));
    } catch (...) {
        return nullptr;
    }
}

char* coolbox_nlp_remove_numbers(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    try {
        return copy_string(ml::nlp::TextProcessor::remove_numbers(text));
    } catch (...) {
        return nullptr;
    }
}

char* coolbox_nlp_normalize_whitespace(const char* text) {
    if (text == nullptr) {
        return nullptr;
    }
    try {
        return copy_string(ml::nlp::TextProcessor::strip(
            ml::nlp::TextProcessor::remove_extra_whitespace(text)));
    } catch (...) {
        return nullptr;
    }
}

void coolbox_nlp_free_string(char* value) {
    std::free(value);
}

void* coolbox_nlp_tokenize(const char* text, int mode) {
    if (text == nullptr || mode < 0 || mode > 3) {
        return nullptr;
    }
    try {
        auto result = std::make_unique<StringVector>();
        ml::nlp::TextProcessor processor;
        switch (mode) {
        case 0:
            result->values = ml::nlp::TextProcessor::tokenize(text);
            break;
        case 1:
            result->values = ml::nlp::TextProcessor::word_tokenize(text);
            break;
        case 2:
            result->values = ml::nlp::TextProcessor::sentence_tokenize(text);
            break;
        case 3:
            result->values = processor.process(text);
            break;
        }
        return result.release();
    } catch (...) {
        return nullptr;
    }
}

size_t coolbox_nlp_token_count(const void* handle) {
    return handle == nullptr ? 0 : static_cast<const StringVector*>(handle)->values.size();
}

char* coolbox_nlp_token_at(const void* handle, size_t index) {
    if (handle == nullptr) {
        return nullptr;
    }
    const auto& values = static_cast<const StringVector*>(handle)->values;
    return index >= values.size() ? nullptr : copy_string(values[index]);
}

void coolbox_nlp_tokens_free(void* handle) {
    delete static_cast<StringVector*>(handle);
}

} // extern "C"
