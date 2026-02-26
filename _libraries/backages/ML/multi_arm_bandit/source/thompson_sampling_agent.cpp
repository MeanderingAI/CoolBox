#include "thompson_sampling_agent.h"
#include <algorithm>

ThompsonSamplingAgent::ThompsonSamplingAgent(const std::vector<double>& true_probs, long long seed)
    : BanditAgent(true_probs), gen(seed) {
    alphas_.assign(true_probs.size(), 1.0);
    betas_.assign(true_probs.size(), 1.0);
}

void ThompsonSamplingAgent::choose_and_pull() {
    int chosen = get_best_sampled_index();
    double reward = arms_[chosen].pull();
    arms_[chosen].update(reward);
    // Update Beta distribution parameters
    if (reward > 0.5) {
        alphas_[chosen] += 1.0;
    } else {
        betas_[chosen] += 1.0;
    }
}

int ThompsonSamplingAgent::get_best_sampled_index() {
    int best = 0;
    double best_sample = -1.0;
    for (size_t i = 0; i < arms_.size(); ++i) {
        // Sample from Beta(alpha_i, beta_i)
        std::gamma_distribution<double> gamma_a(alphas_[i], 1.0);
        std::gamma_distribution<double> gamma_b(betas_[i], 1.0);
        double x = gamma_a(gen);
        double y = gamma_b(gen);
        double sample = x / (x + y);
        if (sample > best_sample) {
            best_sample = sample;
            best = static_cast<int>(i);
        }
    }
    return best;
}
