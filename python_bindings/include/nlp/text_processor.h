#ifndef NLP_TEXT_PROCESSOR_H
#define NLP_TEXT_PROCESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>

namespace ml {
namespace nlp {

// Text preprocessing utilities
class TextProcessor {
public:
    TextProcessor();
    
    // Basic preprocessing
    static std::string to_lowercase(const std::string& text);
    static std::string remove_punctuation(const std::string& text);
    static std::string remove_numbers(const std::string& text);
    static std::string remove_extra_whitespace(const std::string& text);
    static std::string strip(const std::string& text);
    
    // Tokenization
    static std::vector<std::string> tokenize(const std::string& text, const std::string& delimiter = " ");
    static std::vector<std::string> word_tokenize(const std::string& text);
    static std::vector<std::string> sentence_tokenize(const std::string& text);
    
    // N-grams
    static std::vector<std::string> generate_ngrams(const std::vector<std::string>& tokens, size_t n);
    
    // Stemming (simple Porter-like stemmer)
    static std::string stem(const std::string& word);
    static std::vector<std::string> stem_tokens(const std::vector<std::string>& tokens);
    
    // Stop words
    static std::set<std::string> default_stop_words();
    static std::vector<std::string> remove_stop_words(
        const std::vector<std::string>& tokens,
        const std::set<std::string>& stop_words = default_stop_words());
    
    // Full pipeline
    std::vector<std::string> process(const std::string& text,
                                      bool lowercase = true,
                                      bool remove_punct = true,
                                      bool remove_nums = false,
                                      bool remove_stops = true,
                                      bool apply_stemming = false);
};

// Vocabulary management
class Vocabulary {
public:
    Vocabulary(size_t min_freq = 1, size_t max_size = 0);
    
    // Build vocabulary from corpus
    void build(const std::vector<std::vector<std::string>>& documents);
    void build_from_texts(const std::vector<std::string>& texts);
    
    // Add special tokens
    void add_special_token(const std::string& token);
    
    // Token <-> Index mapping
    size_t token_to_index(const std::string& token) const;
    std::string index_to_token(size_t index) const;
    
    bool contains(const std::string& token) const;
    
    // Encode/Decode
    std::vector<size_t> encode(const std::vector<std::string>& tokens) const;
    std::vector<std::string> decode(const std::vector<size_t>& indices) const;
    
    // Stats
    size_t size() const { return token_to_idx_.size(); }
    size_t frequency(const std::string& token) const;
    
    // Special tokens
    static constexpr size_t PAD_IDX = 0;
    static constexpr size_t UNK_IDX = 1;
    static constexpr size_t BOS_IDX = 2;  // Begin of sentence
    static constexpr size_t EOS_IDX = 3;  // End of sentence
    
    static const char* PAD_TOKEN;
    static const char* UNK_TOKEN;
    static const char* BOS_TOKEN;
    static const char* EOS_TOKEN;
    
private:
    std::unordered_map<std::string, size_t> token_to_idx_;
    std::unordered_map<size_t, std::string> idx_to_token_;
    std::unordered_map<std::string, size_t> token_freq_;
    
    size_t min_freq_;
    size_t max_size_;
    size_t next_idx_;
};

// Bag of Words representation
class BagOfWords {
public:
    BagOfWords(std::shared_ptr<Vocabulary> vocab = nullptr);
    
    void fit(const std::vector<std::vector<std::string>>& documents);
    
    std::vector<double> transform(const std::vector<std::string>& tokens) const;
    std::vector<std::vector<double>> transform_batch(
        const std::vector<std::vector<std::string>>& documents) const;
    
    std::shared_ptr<Vocabulary> vocabulary() const { return vocab_; }
    
private:
    std::shared_ptr<Vocabulary> vocab_;
};

// TF-IDF representation
class TFIDF {
public:
    TFIDF(std::shared_ptr<Vocabulary> vocab = nullptr);
    
    void fit(const std::vector<std::vector<std::string>>& documents);
    
    std::vector<double> transform(const std::vector<std::string>& tokens) const;
    std::vector<std::vector<double>> transform_batch(
        const std::vector<std::vector<std::string>>& documents) const;
    
    std::shared_ptr<Vocabulary> vocabulary() const { return vocab_; }
    
private:
    std::shared_ptr<Vocabulary> vocab_;
    std::unordered_map<std::string, double> idf_;
    size_t num_documents_;
    
    void compute_idf(const std::vector<std::vector<std::string>>& documents);
};

// Sequence encoding/decoding
class SequenceEncoder {
public:
    SequenceEncoder(std::shared_ptr<Vocabulary> vocab,
                   size_t max_length = 0,
                   bool padding = true,
                   bool truncation = true);
    
    // Encode single sequence
    std::vector<size_t> encode(const std::vector<std::string>& tokens) const;
    
    // Encode batch with padding
    std::vector<std::vector<size_t>> encode_batch(
        const std::vector<std::vector<std::string>>& sequences) const;
    
    // Decode
    std::vector<std::string> decode(const std::vector<size_t>& indices,
                                     bool skip_special = true) const;
    
private:
    std::shared_ptr<Vocabulary> vocab_;
    size_t max_length_;
    bool padding_;
    bool truncation_;
};

// Character-level encoding
class CharacterEncoder {
public:
    CharacterEncoder();
    
    void fit(const std::vector<std::string>& texts);
    
    std::vector<size_t> encode(const std::string& text) const;
    std::string decode(const std::vector<size_t>& indices) const;
    
    size_t vocab_size() const { return char_to_idx_.size(); }
    
private:
    std::unordered_map<char, size_t> char_to_idx_;
    std::unordered_map<size_t, char> idx_to_char_;
    size_t next_idx_;
};

// Text similarity metrics
double cosine_similarity(const std::vector<double>& vec1, const std::vector<double>& vec2);
double jaccard_similarity(const std::set<std::string>& set1, const std::set<std::string>& set2);
size_t levenshtein_distance(const std::string& s1, const std::string& s2);

} // namespace nlp
} // namespace ml

#endif // NLP_TEXT_PROCESSOR_H
