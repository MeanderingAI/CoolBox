#include "ML/distribution/multinomial_distribution.h"
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <limits>

// A small constant to handle floating-point comparisons for sum of probabilities.
const double EPSILON = 1e-9;

// Helper function to calculate the log of a factorial (log(n!)) using std::lgamma.
// This is used for the log-PMF calculation.
double log_factorial(int n) {
    return std::lgamma(n + 1);
}

// Constructor for the MultinomialDistribution.
MultinomialDistribution::MultinomialDistribution(int trials, const std::vector<double>& probabilities)
    : trials_(trials), 
      probabilities_(probabilities), 
      // Correctly initialize dist_ with a range of iterators
      dist_(probabilities.begin(), probabilities.end()), 
      // Initialize the random number generator
      gen_(std::random_device()())
{
    if (trials <= 0) {
        throw std::invalid_argument("Number of trials must be positive.");
    }
    double sum_p = std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
    if (std::abs(sum_p - 1.0) > EPSILON) {
        throw std::invalid_argument("Probabilities must sum to 1.0.");
    }
}

// Overloaded PDF function to calculate the probability of a specific outcome vector.
// This is the core of the multinomial distribution.
double MultinomialDistribution::pdf(const std::vector<int>& counts) const {
    if (counts.size() != probabilities_.size()) {
        throw std::invalid_argument("The counts vector size must match the probabilities vector size.");
    }
    
    int sum_counts = std::accumulate(counts.begin(), counts.end(), 0);
    if (sum_counts != trials_) {
        throw std::invalid_argument("The sum of counts must equal the number of trials.");
    }

    double log_p = 0.0;
    
    // Calculate the log of the multinomial coefficient: log(n!) - sum(log(x_i!))
    log_p += log_factorial(trials_);
    for (int count : counts) {
        if (count < 0) {
            throw std::invalid_argument("Counts cannot be negative.");
        }
        log_p -= log_factorial(count);
    }

    // Add the term for the probabilities: sum(x_i * log(p_i))
    for (size_t i = 0; i < counts.size(); ++i) {
        if (probabilities_[i] > 0) {
            log_p += counts[i] * std::log(probabilities_[i]);
        } else if (counts[i] > 0) {
            // An outcome with non-zero count has zero probability, so the total probability is 0.
            return 0.0;
        }
    }

    return std::exp(log_p);
}

// The base class methods that are not meaningful for this distribution are implemented below.
double MultinomialDistribution::sample() {
    // This samples a single outcome from one trial and returns it as a double.
    return static_cast<double>(dist_(gen_));
}

// Generates a vector of outcomes from the multinomial distribution.
std::vector<int> MultinomialDistribution::sample_multinomial() {
    std::vector<int> counts(probabilities_.size(), 0);
    for (int i = 0; i < trials_; ++i) {
        int outcome = dist_(gen_);
        counts[outcome]++;
    }
    return counts;
}

// GLM functions are not applicable to the multinomial distribution in the same way.
std::string MultinomialDistribution::link_name() const {
    return "Not Applicable";
}

double MultinomialDistribution::link_function(double mu) const {
    // This function is not applicable for this distribution type.
    return 0.0; 
}

double MultinomialDistribution::mean_function(double eta) const {
    // This function is not applicable for this distribution type.
    return 0.0;
}