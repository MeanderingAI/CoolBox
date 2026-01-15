#include "ML/multi_arm_bandit/thompson_sampling_agent.h"
#include <iostream>

ThompsonSamplingAgent::ThompsonSamplingAgent(const std::vector<double>& true_probs, long long seed)
    : BanditAgent(true_probs), gen(seed) {
    // Initialize alpha and beta for each arm to 1.0 (equivalent to a uniform prior)
    alphas_.resize(true_probs.size(), 1.0);
    betas_.resize(true_probs.size(), 1.0);
}

void ThompsonSamplingAgent::choose_and_pull() {
    int chosen_arm_index = get_best_sampled_index();
    
    double reward = arms_[chosen_arm_index].pull();
    arms_[chosen_arm_index].update(reward);

    // Update alpha and beta based on the reward
    if (reward == 1.0) {
        alphas_[chosen_arm_index] += 1.0;
    } else {
        betas_[chosen_arm_index] += 1.0;
    }
}

int ThompsonSamplingAgent::get_best_sampled_index() {
    double max_sample = -1.0;
    int best_index = -1;

    for (size_t i = 0; i < arms_.size(); ++i) {
        // Draw a random sample from the Beta distribution for each arm
        std::gamma_distribution<> gamma_alpha(alphas_[i], 1.0);
        std::gamma_distribution<> gamma_beta(betas_[i], 1.0);
        
        double sample_alpha = gamma_alpha(gen);
        double sample_beta = gamma_beta(gen);
        
        double sampled_value = sample_alpha / (sample_alpha + sample_beta);

        if (sampled_value > max_sample) {
            max_sample = sampled_value;
            best_index = i;
        }
    }
    return best_index;
}