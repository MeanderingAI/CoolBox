#ifndef LATENT_SENTIMENT_ANALYSIS_H
#define LATENT_SENTIMENT_ANALYSIS_H

#include <Eigen/Dense>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <cmath>

/**
 * @class LatentSentimentAnalyzer
 * @brief Implements a simplified Latent Sentiment Analysis model
 * using Matrix Factorization via Stochastic Gradient Descent (SGD), 
 * leveraging the Eigen library for efficient linear algebra.
 */
class LatentSentimentAnalysis {
public:
	/**
	 * @brief Constructor for the analyzer.
	 * @param latent_features Number of latent features (e.g. 2 for Pos/Neg)
	 * @param learning_rate SGD learning rate.
	 * @param lambda Regularization parameter.
	 * @param max_iterations Maximum iterations for training.
	 */
	LatentSentimentAnalysis(
		int latent_features = 2,
		double learning_rate = 0.01,
		double lambda = 0.1,
		int max_iterations = 1000
	);

    /**
     * @brief Trains the model using a Document-Term Matrix (M).
     * @param document_term_matrix The input matrix. It must be convertible to Eigen::MatrixXd.
     */
    void train(const Eigen::MatrixXd& document_term_matrix);

    /**
     * @brief Predicts the reconstructed value for a given document and term (U_i * V_j^T).
     * @param doc_index The index of the document (row in U).
     * @param term_index The index of the term (row in V).
     * @return The predicted score.
     */
    double predict_score(int doc_index, int term_index) const;

    /**
     * @brief Gets the Document-Feature Matrix (U).
     * @return U matrix. Rows are documents, columns are latent features.
     */
    const Eigen::MatrixXd& get_document_features() const { return U; }

    /**
     * @brief Gets the Term-Feature Matrix (V).
     * @return V matrix. Rows are terms, columns are latent features.
     */
    const Eigen::MatrixXd& get_term_features() const { return V; }

private:
    int K; // Number of latent features
    double alpha; // Learning rate
    double lambda; // Regularization parameter
    int max_iter; // Max training iterations

    int num_documents;
    int num_terms;

    // U: Document-Feature Matrix (D x K)
    Eigen::MatrixXd U; 
    
    // V: Term-Feature Matrix (T x K). The factor for the term side.
    Eigen::MatrixXd V; 

    /**
     * @brief Initializes the matrices U and V using Eigen's random functions.
     */
    void initialize_matrices(int rows_U, int rows_V);

    /**
     * @brief Performs one step of Stochastic Gradient Descent for a single entry M(i, j).
     * @param i Document index.
     * @param j Term index.
     * @param error The current prediction error (M(i, j) - prediction).
     * @param M Reference to the Document-Term Matrix.
     */
    void sgd_step(int i, int j, double error);
};

#endif // LATENT_SENTIMENT_ANALYSIS_H