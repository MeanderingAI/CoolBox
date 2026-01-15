#include "ML/nlp/embeddings.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace ml {
namespace nlp {

// WordEmbedding implementation
WordEmbedding::WordEmbedding(size_t embedding_dim)
    : embedding_dim_(embedding_dim) {
    // Initialize unknown embedding to zeros
    unknown_embedding_.resize(embedding_dim, 0.0);
}

void WordEmbedding::random_init(const std::vector<std::string>& vocabulary) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-0.1, 0.1);
    
    for (const auto& word : vocabulary) {
        std::vector<double> embedding(embedding_dim_);
        for (auto& val : embedding) {
            val = dis(gen);
        }
        embeddings_[word] = embedding;
    }
}

void WordEmbedding::xavier_init(const std::vector<std::string>& vocabulary) {
    std::random_device rd;
    std::mt19937 gen(rd());
    double limit = std::sqrt(6.0 / embedding_dim_);
    std::uniform_real_distribution<> dis(-limit, limit);
    
    for (const auto& word : vocabulary) {
        std::vector<double> embedding(embedding_dim_);
        for (auto& val : embedding) {
            val = dis(gen);
        }
        embeddings_[word] = embedding;
    }
}

void WordEmbedding::load_pretrained(const std::string& filename) {
    // Stub - in practice would read from file (e.g., GloVe, Word2Vec format)
    throw std::runtime_error("load_pretrained not yet implemented");
}

std::vector<double> WordEmbedding::get_embedding(const std::string& word) const {
    auto it = embeddings_.find(word);
    if (it != embeddings_.end()) {
        return it->second;
    }
    return unknown_embedding_;
}

bool WordEmbedding::has_word(const std::string& word) const {
    return embeddings_.find(word) != embeddings_.end();
}

std::vector<std::vector<double>> WordEmbedding::get_embeddings(const std::vector<std::string>& words) const {
    std::vector<std::vector<double>> result;
    result.reserve(words.size());
    for (const auto& word : words) {
        result.push_back(get_embedding(word));
    }
    return result;
}

void WordEmbedding::update_embedding(const std::string& word, const std::vector<double>& new_embedding) {
    if (new_embedding.size() != embedding_dim_) {
        throw std::invalid_argument("Embedding dimension mismatch");
    }
    embeddings_[word] = new_embedding;
}

double WordEmbedding::similarity(const std::string& word1, const std::string& word2) const {
    auto emb1 = get_embedding(word1);
    auto emb2 = get_embedding(word2);
    
    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (size_t i = 0; i < embedding_dim_; ++i) {
        dot += emb1[i] * emb2[i];
        norm1 += emb1[i] * emb1[i];
        norm2 += emb2[i] * emb2[i];
    }
    
    if (norm1 == 0.0 || norm2 == 0.0) return 0.0;
    return dot / (std::sqrt(norm1) * std::sqrt(norm2));
}

std::vector<std::pair<std::string, double>> WordEmbedding::most_similar(
    const std::string& word, size_t top_k) const {
    
    if (!has_word(word)) {
        return {};
    }
    
    auto target_emb = get_embedding(word);
    std::vector<std::pair<std::string, double>> similarities;
    
    for (const auto& [other_word, emb] : embeddings_) {
        if (other_word == word) continue;
        
        double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
        for (size_t i = 0; i < embedding_dim_; ++i) {
            dot += target_emb[i] * emb[i];
            norm1 += target_emb[i] * target_emb[i];
            norm2 += emb[i] * emb[i];
        }
        
        double sim = 0.0;
        if (norm1 > 0.0 && norm2 > 0.0) {
            sim = dot / (std::sqrt(norm1) * std::sqrt(norm2));
        }
        
        similarities.push_back({other_word, sim});
    }
    
    // Sort by similarity descending
    std::partial_sort(similarities.begin(), 
                     similarities.begin() + std::min(top_k, similarities.size()),
                     similarities.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (similarities.size() > top_k) {
        similarities.resize(top_k);
    }
    
    return similarities;
}

// OneHotEncoder implementation
OneHotEncoder::OneHotEncoder() : vocab_size_(0) {}

void OneHotEncoder::fit(const std::vector<std::string>& vocabulary) {
    vocab_size_ = vocabulary.size();
    for (size_t i = 0; i < vocabulary.size(); ++i) {
        word_to_idx_[vocabulary[i]] = i;
    }
}

std::vector<double> OneHotEncoder::encode(const std::string& word) const {
    std::vector<double> encoding(vocab_size_, 0.0);
    
    auto it = word_to_idx_.find(word);
    if (it != word_to_idx_.end()) {
        encoding[it->second] = 1.0;
    }
    
    return encoding;
}

std::vector<std::vector<double>> OneHotEncoder::encode_batch(const std::vector<std::string>& words) const {
    std::vector<std::vector<double>> results;
    results.reserve(words.size());
    for (const auto& word : words) {
        results.push_back(encode(word));
    }
    return results;
}

// Positional encoding for transformers
std::vector<std::vector<double>> create_positional_encoding(
    size_t max_length, size_t embedding_dim) {
    
    std::vector<std::vector<double>> pe(max_length, std::vector<double>(embedding_dim));
    
    for (size_t pos = 0; pos < max_length; ++pos) {
        for (size_t i = 0; i < embedding_dim; ++i) {
            double angle = pos / std::pow(10000.0, (2.0 * i) / embedding_dim);
            if (i % 2 == 0) {
                pe[pos][i] = std::sin(angle);
            } else {
                pe[pos][i] = std::cos(angle);
            }
        }
    }
    
    return pe;
}

// Embedding utilities
std::vector<double> average_embeddings(const std::vector<std::vector<double>>& embeddings) {
    if (embeddings.empty()) return {};
    
    size_t dim = embeddings[0].size();
    std::vector<double> avg(dim, 0.0);
    
    for (const auto& emb : embeddings) {
        for (size_t i = 0; i < dim && i < emb.size(); ++i) {
            avg[i] += emb[i];
        }
    }
    
    for (auto& val : avg) {
        val /= embeddings.size();
    }
    
    return avg;
}

std::vector<double> max_pooling_embeddings(const std::vector<std::vector<double>>& embeddings) {
    if (embeddings.empty()) return {};
    
    size_t dim = embeddings[0].size();
    std::vector<double> max_pool(dim, -std::numeric_limits<double>::infinity());
    
    for (const auto& emb : embeddings) {
        for (size_t i = 0; i < dim && i < emb.size(); ++i) {
            max_pool[i] = std::max(max_pool[i], emb[i]);
        }
    }
    
    return max_pool;
}

} // namespace nlp
} // namespace ml
