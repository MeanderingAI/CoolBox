#ifndef NLP_EMBEDDINGS_H
#define NLP_EMBEDDINGS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace ml {
namespace nlp {

// Simple word embedding class
class WordEmbedding {
public:
    WordEmbedding(size_t embedding_dim);
    
    // Initialize embeddings
    void random_init(const std::vector<std::string>& vocabulary);
    void xavier_init(const std::vector<std::string>& vocabulary);
    
    // Load pre-trained (stub - would load from file)
    void load_pretrained(const std::string& filename);
    
    // Get embedding
    std::vector<double> get_embedding(const std::string& word) const;
    bool has_word(const std::string& word) const;
    
    // Get embedding matrix for batch
    std::vector<std::vector<double>> get_embeddings(const std::vector<std::string>& words) const;
    
    // Update embedding (for training)
    void update_embedding(const std::string& word, const std::vector<double>& new_embedding);
    
    // Vocabulary
    size_t vocab_size() const { return embeddings_.size(); }
    size_t embedding_dim() const { return embedding_dim_; }
    
    // Similarity
    double similarity(const std::string& word1, const std::string& word2) const;
    std::vector<std::pair<std::string, double>> most_similar(
        const std::string& word, size_t top_k = 10) const;
    
private:
    size_t embedding_dim_;
    std::unordered_map<std::string, std::vector<double>> embeddings_;
    std::vector<double> unknown_embedding_;
};

// One-hot encoding
class OneHotEncoder {
public:
    OneHotEncoder();
    
    void fit(const std::vector<std::string>& vocabulary);
    
    std::vector<double> encode(const std::string& word) const;
    std::vector<std::vector<double>> encode_batch(const std::vector<std::string>& words) const;
    
    size_t vocab_size() const { return vocab_size_; }
    
private:
    std::unordered_map<std::string, size_t> word_to_idx_;
    size_t vocab_size_;
};

// Positional encoding for transformers
std::vector<std::vector<double>> create_positional_encoding(
    size_t max_length, size_t embedding_dim);

// Embedding utilities
std::vector<double> average_embeddings(const std::vector<std::vector<double>>& embeddings);
std::vector<double> max_pooling_embeddings(const std::vector<std::vector<double>>& embeddings);

} // namespace nlp
} // namespace ml

#endif // NLP_EMBEDDINGS_H
