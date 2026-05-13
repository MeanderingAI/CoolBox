#include "latent_sentiment_analysis.h"
#include <random>

LatentSentimentAnalysis::LatentSentimentAnalysis(int latent_features,
    double learning_rate, double lambda, int max_iterations)
    : K(latent_features), alpha(learning_rate), lambda(lambda),
      max_iter(max_iterations), num_documents(0), num_terms(0) {}

void LatentSentimentAnalysis::initialize_matrices(int rows_U, int rows_V) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    U = matrix::DenseMatrix(rows_U, K);
    V = matrix::DenseMatrix(rows_V, K);
    for (int i = 0; i < rows_U; ++i)
        for (int j = 0; j < K; ++j)
            U.at(i, j) = dist(gen) * 0.1;
    for (int i = 0; i < rows_V; ++i)
        for (int j = 0; j < K; ++j)
            V.at(i, j) = dist(gen) * 0.1;
}

void LatentSentimentAnalysis::sgd_step(int i, int j, double error) {
    for (int k = 0; k < K; ++k) {
        double u_ik = U.at(i, k);
        double v_jk = V.at(j, k);
        U.at(i, k) += alpha * (error * v_jk - lambda * u_ik);
        V.at(j, k) += alpha * (error * u_ik - lambda * v_jk);
    }
}

void LatentSentimentAnalysis::train(const matrix::DenseMatrix& document_term_matrix) {
    num_documents = document_term_matrix.rows();
    num_terms = document_term_matrix.cols();
    initialize_matrices(num_documents, num_terms);
    for (int iter = 0; iter < max_iter; ++iter) {
        for (int i = 0; i < num_documents; ++i) {
            for (int j = 0; j < num_terms; ++j) {
                double observed = document_term_matrix.at(i, j);
                double predicted = 0.0;
                for (int k = 0; k < K; ++k) predicted += U.at(i, k) * V.at(j, k);
                double error = observed - predicted;
                sgd_step(i, j, error);
            }
        }
    }
}

double LatentSentimentAnalysis::predict_score(int doc_index, int term_index) const {
    if (doc_index < 0 || doc_index >= num_documents ||
        term_index < 0 || term_index >= num_terms) {
        return 0.0;
    }
    double score = 0.0;
    for (int k = 0; k < K; ++k) score += U.at(doc_index, k) * V.at(term_index, k);
    return score;
}
