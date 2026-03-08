#include "latent_sentiment_analysis.h"
#include <random>

LatentSentimentAnalysis::LatentSentimentAnalysis(int latent_features,
    double learning_rate, double lambda, int max_iterations)
    : K(latent_features), alpha(learning_rate), lambda(lambda),
      max_iter(max_iterations), num_documents(0), num_terms(0) {}

void LatentSentimentAnalysis::initialize_matrices(Eigen::Index rows_U, Eigen::Index rows_V) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    U = Eigen::MatrixXd(rows_U, K);
    V = Eigen::MatrixXd(rows_V, K);
    for (Eigen::Index i = 0; i < rows_U; ++i)
        for (int j = 0; j < K; ++j)
            U(i, j) = dist(gen) * 0.1;
    for (Eigen::Index i = 0; i < rows_V; ++i)
        for (int j = 0; j < K; ++j)
            V(i, j) = dist(gen) * 0.1;
}

void LatentSentimentAnalysis::sgd_step(Eigen::Index i, Eigen::Index j, double error) {
    Eigen::VectorXd u_i = U.row(i);
    Eigen::VectorXd v_j = V.row(j);
    U.row(i) += alpha * (error * v_j.transpose() - lambda * u_i.transpose());
    V.row(j) += alpha * (error * u_i.transpose() - lambda * v_j.transpose());
}

void LatentSentimentAnalysis::train(const Eigen::MatrixXd& document_term_matrix) {
    num_documents = document_term_matrix.rows();
    num_terms = document_term_matrix.cols();
    initialize_matrices(num_documents, num_terms);

    for (int iter = 0; iter < max_iter; ++iter) {
        for (Eigen::Index i = 0; i < num_documents; ++i) {
            for (Eigen::Index j = 0; j < num_terms; ++j) {
                if (document_term_matrix(i, j) > 0) {
                    double pred = U.row(i).dot(V.row(j));
                    double error = document_term_matrix(i, j) - pred;
                    sgd_step(i, j, error);
                }
            }
        }
    }
}

double LatentSentimentAnalysis::predict_score(Eigen::Index doc_index, Eigen::Index term_index) const {
    if (doc_index < 0 || doc_index >= num_documents ||
        term_index < 0 || term_index >= num_terms) {
        return 0.0;
    }
    return U.row(doc_index).dot(V.row(term_index));
}
