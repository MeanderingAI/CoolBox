#include "ml/nlp/text_processor.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <regex>

namespace ml {
namespace nlp {

// Static constants for Vocabulary
const char* Vocabulary::PAD_TOKEN = "<PAD>";
const char* Vocabulary::UNK_TOKEN = "<UNK>";
const char* Vocabulary::BOS_TOKEN = "<BOS>";
const char* Vocabulary::EOS_TOKEN = "<EOS>";

// TextProcessor implementation
TextProcessor::TextProcessor() {}

std::string TextProcessor::to_lowercase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string TextProcessor::remove_punctuation(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        if (!std::ispunct(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

std::string TextProcessor::remove_numbers(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

std::string TextProcessor::remove_extra_whitespace(const std::string& text) {
    std::string result;
    bool prev_space = false;
    
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!prev_space) {
                result += ' ';
                prev_space = true;
            }
        } else {
            result += c;
            prev_space = false;
        }
    }
    
    return result;
}

std::string TextProcessor::strip(const std::string& text) {
    auto start = std::find_if_not(text.begin(), text.end(),
                                   [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(text.rbegin(), text.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}

std::vector<std::string> TextProcessor::tokenize(const std::string& text, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = text.find(delimiter);
    
    while (end != std::string::npos) {
        if (end > start) {
            tokens.push_back(text.substr(start, end - start));
        }
        start = end + delimiter.length();
        end = text.find(delimiter, start);
    }
    
    if (start < text.length()) {
        tokens.push_back(text.substr(start));
    }
    
    return tokens;
}

std::vector<std::string> TextProcessor::word_tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) || std::ispunct(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

std::vector<std::string> TextProcessor::sentence_tokenize(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current;
    
    for (size_t i = 0; i < text.size(); ++i) {
        current += text[i];
        
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            if (i + 1 >= text.size() || std::isspace(text[i + 1])) {
                sentences.push_back(strip(current));
                current.clear();
            }
        }
    }
    
    if (!current.empty()) {
        sentences.push_back(strip(current));
    }
    
    return sentences;
}

std::vector<std::string> TextProcessor::generate_ngrams(const std::vector<std::string>& tokens, size_t n) {
    std::vector<std::string> ngrams;
    
    if (tokens.size() < n) return ngrams;
    
    for (size_t i = 0; i <= tokens.size() - n; ++i) {
        std::string ngram = tokens[i];
        for (size_t j = 1; j < n; ++j) {
            ngram += " " + tokens[i + j];
        }
        ngrams.push_back(ngram);
    }
    
    return ngrams;
}

std::string TextProcessor::stem(const std::string& word) {
    // Simple stemming rules (very basic Porter-like)
    std::string result = word;
    
    if (result.length() <= 2) return result;
    
    // Remove common suffixes
    if (result.length() > 4 && result.substr(result.length() - 3) == "ing") {
        result = result.substr(0, result.length() - 3);
    } else if (result.length() > 3 && result.substr(result.length() - 2) == "ed") {
        result = result.substr(0, result.length() - 2);
    } else if (result.length() > 3 && result.substr(result.length() - 1) == "s") {
        result = result.substr(0, result.length() - 1);
    }
    
    return result;
}

std::vector<std::string> TextProcessor::stem_tokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> stemmed;
    stemmed.reserve(tokens.size());
    for (const auto& token : tokens) {
        stemmed.push_back(stem(token));
    }
    return stemmed;
}

std::set<std::string> TextProcessor::default_stop_words() {
    return {
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
        "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
        "to", "was", "will", "with", "the", "this", "but", "they", "have",
        "had", "what", "when", "where", "who", "which", "why", "how"
    };
}

std::vector<std::string> TextProcessor::remove_stop_words(
    const std::vector<std::string>& tokens,
    const std::set<std::string>& stop_words) {
    
    std::vector<std::string> filtered;
    for (const auto& token : tokens) {
        if (stop_words.find(to_lowercase(token)) == stop_words.end()) {
            filtered.push_back(token);
        }
    }
    return filtered;
}

std::vector<std::string> TextProcessor::process(
    const std::string& text,
    bool lowercase,
    bool remove_punct,
    bool remove_nums,
    bool remove_stops,
    bool apply_stemming) {
    
    std::string processed = text;
    
    if (lowercase) {
        processed = to_lowercase(processed);
    }
    
    if (remove_punct) {
        processed = remove_punctuation(processed);
    }
    
    if (remove_nums) {
        processed = remove_numbers(processed);
    }
    
    processed = remove_extra_whitespace(processed);
    processed = strip(processed);
    
    auto tokens = word_tokenize(processed);
    
    if (remove_stops) {
        tokens = remove_stop_words(tokens);
    }
    
    if (apply_stemming) {
        tokens = stem_tokens(tokens);
    }
    
    return tokens;
}

// Vocabulary implementation
Vocabulary::Vocabulary(size_t min_freq, size_t max_size)
    : min_freq_(min_freq), max_size_(max_size), next_idx_(4) {
    
    // Add special tokens
    token_to_idx_[PAD_TOKEN] = PAD_IDX;
    token_to_idx_[UNK_TOKEN] = UNK_IDX;
    token_to_idx_[BOS_TOKEN] = BOS_IDX;
    token_to_idx_[EOS_TOKEN] = EOS_IDX;
    
    idx_to_token_[PAD_IDX] = PAD_TOKEN;
    idx_to_token_[UNK_IDX] = UNK_TOKEN;
    idx_to_token_[BOS_IDX] = BOS_TOKEN;
    idx_to_token_[EOS_IDX] = EOS_TOKEN;
}

void Vocabulary::build(const std::vector<std::vector<std::string>>& documents) {
    // Count frequencies
    for (const auto& doc : documents) {
        for (const auto& token : doc) {
            token_freq_[token]++;
        }
    }
    
    // Sort by frequency
    std::vector<std::pair<std::string, size_t>> freq_vec(token_freq_.begin(), token_freq_.end());
    std::sort(freq_vec.begin(), freq_vec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Build vocabulary
    for (const auto& [token, freq] : freq_vec) {
        if (freq < min_freq_) break;
        if (max_size_ > 0 && token_to_idx_.size() >= max_size_ + 4) break;
        
        if (token_to_idx_.find(token) == token_to_idx_.end()) {
            token_to_idx_[token] = next_idx_;
            idx_to_token_[next_idx_] = token;
            next_idx_++;
        }
    }
}

void Vocabulary::build_from_texts(const std::vector<std::string>& texts) {
    TextProcessor processor;
    std::vector<std::vector<std::string>> documents;
    
    for (const auto& text : texts) {
        documents.push_back(processor.word_tokenize(text));
    }
    
    build(documents);
}

void Vocabulary::add_special_token(const std::string& token) {
    if (token_to_idx_.find(token) == token_to_idx_.end()) {
        token_to_idx_[token] = next_idx_;
        idx_to_token_[next_idx_] = token;
        next_idx_++;
    }
}

size_t Vocabulary::token_to_index(const std::string& token) const {
    auto it = token_to_idx_.find(token);
    return (it != token_to_idx_.end()) ? it->second : UNK_IDX;
}

std::string Vocabulary::index_to_token(size_t index) const {
    auto it = idx_to_token_.find(index);
    return (it != idx_to_token_.end()) ? it->second : UNK_TOKEN;
}

bool Vocabulary::contains(const std::string& token) const {
    return token_to_idx_.find(token) != token_to_idx_.end();
}

std::vector<size_t> Vocabulary::encode(const std::vector<std::string>& tokens) const {
    std::vector<size_t> indices;
    indices.reserve(tokens.size());
    for (const auto& token : tokens) {
        indices.push_back(token_to_index(token));
    }
    return indices;
}

std::vector<std::string> Vocabulary::decode(const std::vector<size_t>& indices) const {
    std::vector<std::string> tokens;
    tokens.reserve(indices.size());
    for (size_t idx : indices) {
        tokens.push_back(index_to_token(idx));
    }
    return tokens;
}

size_t Vocabulary::frequency(const std::string& token) const {
    auto it = token_freq_.find(token);
    return (it != token_freq_.end()) ? it->second : 0;
}

// BagOfWords implementation
BagOfWords::BagOfWords(std::shared_ptr<Vocabulary> vocab)
    : vocab_(vocab ? vocab : std::make_shared<Vocabulary>()) {}

void BagOfWords::fit(const std::vector<std::vector<std::string>>& documents) {
    if (!vocab_) {
        vocab_ = std::make_shared<Vocabulary>();
    }
    vocab_->build(documents);
}

std::vector<double> BagOfWords::transform(const std::vector<std::string>& tokens) const {
    std::vector<double> bow(vocab_->size(), 0.0);
    
    for (const auto& token : tokens) {
        if (vocab_->contains(token)) {
            size_t idx = vocab_->token_to_index(token);
            if (idx < bow.size()) {
                bow[idx]++;
            }
        }
    }
    
    return bow;
}

std::vector<std::vector<double>> BagOfWords::transform_batch(
    const std::vector<std::vector<std::string>>& documents) const {
    
    std::vector<std::vector<double>> results;
    results.reserve(documents.size());
    for (const auto& doc : documents) {
        results.push_back(transform(doc));
    }
    return results;
}

// TFIDF implementation
TFIDF::TFIDF(std::shared_ptr<Vocabulary> vocab)
    : vocab_(vocab ? vocab : std::make_shared<Vocabulary>()), num_documents_(0) {}

void TFIDF::fit(const std::vector<std::vector<std::string>>& documents) {
    if (!vocab_) {
        vocab_ = std::make_shared<Vocabulary>();
    }
    vocab_->build(documents);
    compute_idf(documents);
}

void TFIDF::compute_idf(const std::vector<std::vector<std::string>>& documents) {
    num_documents_ = documents.size();
    std::unordered_map<std::string, size_t> doc_freq;
    
    for (const auto& doc : documents) {
        std::set<std::string> unique_tokens(doc.begin(), doc.end());
        for (const auto& token : unique_tokens) {
            doc_freq[token]++;
        }
    }
    
    for (const auto& [token, freq] : doc_freq) {
        idf_[token] = std::log(static_cast<double>(num_documents_) / freq);
    }
}

std::vector<double> TFIDF::transform(const std::vector<std::string>& tokens) const {
    std::vector<double> tfidf(vocab_->size(), 0.0);
    
    // Calculate term frequency
    std::unordered_map<std::string, double> tf;
    for (const auto& token : tokens) {
        tf[token]++;
    }
    
    // Normalize by document length
    for (auto& [token, freq] : tf) {
        freq /= tokens.size();
    }
    
    // Calculate TF-IDF
    for (const auto& [token, freq] : tf) {
        if (vocab_->contains(token)) {
            size_t idx = vocab_->token_to_index(token);
            auto idf_it = idf_.find(token);
            double idf_val = (idf_it != idf_.end()) ? idf_it->second : 0.0;
            if (idx < tfidf.size()) {
                tfidf[idx] = freq * idf_val;
            }
        }
    }
    
    return tfidf;
}

std::vector<std::vector<double>> TFIDF::transform_batch(
    const std::vector<std::vector<std::string>>& documents) const {
    
    std::vector<std::vector<double>> results;
    results.reserve(documents.size());
    for (const auto& doc : documents) {
        results.push_back(transform(doc));
    }
    return results;
}

// SequenceEncoder implementation
SequenceEncoder::SequenceEncoder(std::shared_ptr<Vocabulary> vocab,
                                 size_t max_length,
                                 bool padding,
                                 bool truncation)
    : vocab_(vocab), max_length_(max_length), padding_(padding), truncation_(truncation) {}

std::vector<size_t> SequenceEncoder::encode(const std::vector<std::string>& tokens) const {
    std::vector<size_t> indices = vocab_->encode(tokens);
    
    if (max_length_ > 0) {
        if (truncation_ && indices.size() > max_length_) {
            indices.resize(max_length_);
        } else if (padding_ && indices.size() < max_length_) {
            indices.resize(max_length_, Vocabulary::PAD_IDX);
        }
    }
    
    return indices;
}

std::vector<std::vector<size_t>> SequenceEncoder::encode_batch(
    const std::vector<std::vector<std::string>>& sequences) const {
    
    std::vector<std::vector<size_t>> results;
    results.reserve(sequences.size());
    
    for (const auto& seq : sequences) {
        results.push_back(encode(seq));
    }
    
    return results;
}

std::vector<std::string> SequenceEncoder::decode(const std::vector<size_t>& indices,
                                                  bool skip_special) const {
    std::vector<std::string> tokens = vocab_->decode(indices);
    
    if (skip_special) {
        tokens.erase(
            std::remove_if(tokens.begin(), tokens.end(),
                          [](const std::string& t) {
                              return t == Vocabulary::PAD_TOKEN || 
                                     t == Vocabulary::BOS_TOKEN ||
                                     t == Vocabulary::EOS_TOKEN;
                          }),
            tokens.end());
    }
    
    return tokens;
}

// CharacterEncoder implementation
CharacterEncoder::CharacterEncoder() : next_idx_(0) {}

void CharacterEncoder::fit(const std::vector<std::string>& texts) {
    std::set<char> unique_chars;
    for (const auto& text : texts) {
        for (char c : text) {
            unique_chars.insert(c);
        }
    }
    
    for (char c : unique_chars) {
        char_to_idx_[c] = next_idx_;
        idx_to_char_[next_idx_] = c;
        next_idx_++;
    }
}

std::vector<size_t> CharacterEncoder::encode(const std::string& text) const {
    std::vector<size_t> indices;
    indices.reserve(text.size());
    for (char c : text) {
        auto it = char_to_idx_.find(c);
        if (it != char_to_idx_.end()) {
            indices.push_back(it->second);
        }
    }
    return indices;
}

std::string CharacterEncoder::decode(const std::vector<size_t>& indices) const {
    std::string text;
    text.reserve(indices.size());
    for (size_t idx : indices) {
        auto it = idx_to_char_.find(idx);
        if (it != idx_to_char_.end()) {
            text += it->second;
        }
    }
    return text;
}

// Utility functions
double cosine_similarity(const std::vector<double>& vec1, const std::vector<double>& vec2) {
    if (vec1.size() != vec2.size()) return 0.0;
    
    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (size_t i = 0; i < vec1.size(); ++i) {
        dot += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }
    
    if (norm1 == 0.0 || norm2 == 0.0) return 0.0;
    return dot / (std::sqrt(norm1) * std::sqrt(norm2));
}

double jaccard_similarity(const std::set<std::string>& set1, const std::set<std::string>& set2) {
    std::set<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                         std::inserter(intersection, intersection.begin()));
    
    std::set<std::string> union_set;
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                  std::inserter(union_set, union_set.begin()));
    
    if (union_set.empty()) return 0.0;
    return static_cast<double>(intersection.size()) / union_set.size();
}

size_t levenshtein_distance(const std::string& s1, const std::string& s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();
    
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));
    
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;
    
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,      // deletion
                dp[i][j - 1] + 1,      // insertion
                dp[i - 1][j - 1] + cost // substitution
            });
        }
    }
    
    return dp[m][n];
}

} // namespace nlp
} // namespace ml
