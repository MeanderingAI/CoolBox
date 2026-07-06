#include "multinomial_distribution.h"
#include "util.h"
#include <cmath>
#include <numeric>

MultinomialDistribution::MultinomialDistribution(int trials, const std::vector<double>& probabilities)
    : trials_(trials), probabilities_(probabilities),
      dist_(probabilities.begin(), probabilities.end()), gen_(std::random_device{}()) {
    // Normalize probabilities
    double sum = std::accumulate(probabilities_.begin(), probabilities_.end(), 0.0);
    if (sum > 0.0) {
        for (auto& p : probabilities_) p /= sum;
    }
}

double MultinomialDistribution::pdf(const std::vector<int>& counts) const {
    if (counts.size() != probabilities_.size()) return 0.0;
    int total = std::accumulate(counts.begin(), counts.end(), 0);
    if (total != trials_) return 0.0;

    double result = static_cast<double>(factorial(trials_));
    for (size_t i = 0; i < counts.size(); ++i) {
        if (counts[i] < 0) return 0.0;
        result /= static_cast<double>(factorial(counts[i]));
        result *= std::pow(probabilities_[i], counts[i]);
    }
    return result;
}

double MultinomialDistribution::sample() {
    return static_cast<double>(dist_(gen_));
}

std::vector<int> MultinomialDistribution::sample_multinomial() {
    std::vector<int> counts(probabilities_.size(), 0);
    for (int i = 0; i < trials_; ++i) {
        int outcome = dist_(gen_);
        counts[outcome]++;
    }
    return counts;
}

std::string MultinomialDistribution::link_name() const {
    return "none";
}

double MultinomialDistribution::link_function(double mu) const {
    return mu;
}

double MultinomialDistribution::mean_function(double eta) const {
    return eta;
}
