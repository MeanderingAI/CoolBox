#include <latent_sentiment_analysis.h>

#include <random>
#include <chrono>

using namespace Eigen;
using namespace std;

// --- Constructor ---
LatentSentimentAnalysis::LatentSentimentAnalysis(
    int latent_features, 
    double learning_rate, 
    double lambda, 
    int max_iterations)
    : K(latent_features), alpha(learning_rate), lambda(lambda), max_iter(max_iterations), 
      num_documents(0), num_terms(0) {
    
    // Seed the random number generator
    srand(static_cast<unsigned int>(time(0))); 
}

// --- Matrix Initialization ---
void LatentSentimentAnalysis::initialize_matrices(int rows_U, int rows_V) {
    num_documents = rows_U;
    num_terms = rows_V;

    // Initialize U (Document-Feature) and V (Term-Feature) matrices
    U.resize(num_documents, K);
    V.resize(num_terms, K);

    // Use Eigen's setRandom() scaled down for initialization
    // Scaling by 0.1 ensures small, random starting values
    U = MatrixXd::Random(num_documents, K) * 0.1;
    V = MatrixXd::Random(num_terms, K) * 0.1;
}

// --- SGD Update Step ---
void LatentSentimentAnalysis::sgd_step(int i, int j, double error) {
    // Perform the update for each latent feature k
    for (int k = 0; k < K; ++k) {
        double u_ik = U(i, k);
        double v_jk = V(j, k);

        // Update U(i, k): Gradient + Regularization Term
        U(i, k) += alpha * (error * v_jk - lambda * u_ik);
        
        // Update V(j, k): Gradient + Regularization Term
        V(j, k) += alpha * (error * u_ik - lambda * v_jk);
    }
}

// --- Prediction ---
double LatentSentimentAnalysis::predict_score(int doc_index, int term_index) const {
    // Prediction is the dot product of the i-th row of U and the j-th row of V
    // U.row(doc_index) is a 1xK vector
    // V.row(term_index) is a 1xK vector
    // The dot product gives the predicted scalar score
    return U.row(doc_index).dot(V.row(term_index));
}

// --- Training Loop ---
void LatentSentimentAnalysis::train(const Eigen::MatrixXd& M) {
    // Initialize dimensions
    int D = M.rows();
    int T = M.cols();

    // Initialize the factor matrices U and V
    initialize_matrices(D, T);

    cout << "Starting training with " << D << " documents and " << T << " terms over " << K << " latent features..." << endl;

    // Training loop
    for (int iter = 0; iter < max_iter; ++iter) {
        double total_error = 0.0;
        int observed_count = 0;

        // Iterate over all entries in the Document-Term Matrix (M)
        for (int i = 0; i < D; ++i) { // Document index
            for (int j = 0; j < T; ++j) { // Term index
                
                // Only consider non-zero entries (observed term frequencies)
                if (M(i, j) > 0) {
                    observed_count++;
                    
                    // 1. Prediction
                    double prediction = predict_score(i, j);

                    // 2. Error Calculation
                    double error = M(i, j) - prediction;
                    total_error += error * error;

                    // 3. Update U and V using SGD
                    sgd_step(i, j, error);
                }
            }
        }
        
        // --- Calculate and Report RMSE ---
        if (iter % 100 == 0 || iter == max_iter - 1) {
            double rmse = 0.0;
            if (observed_count > 0) {
                // Add regularization penalty to the total error (Loss function L = E + lambda*R)
                double regularization_penalty = (U.squaredNorm() + V.squaredNorm()) * lambda;
                rmse = sqrt((total_error + regularization_penalty) / observed_count);
            }
            cout << "Iteration " << setw(5) << iter << ", RMSE: " << fixed << setprecision(6) << rmse << endl;
        }
    }
    cout << "Training complete." << endl;
}
