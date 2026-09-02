#ifndef DYNAMIC_BAYESIAN_NETWORK_H
#define DYNAMIC_BAYESIAN_NETWORK_H

#include <string>
#include <vector>

namespace ml {

class DynamicBayesianNetwork {
public:
    DynamicBayesianNetwork();

    void set_states(const std::vector<std::string>& states);
    const std::vector<std::string>& get_states() const;

    void set_initial_distribution(const std::vector<double>& initial_distribution);
    const std::vector<double>& get_initial_distribution() const;

    void set_transition_matrix(const std::vector<std::vector<double>>& transition_matrix);
    const std::vector<std::vector<double>>& get_transition_matrix() const;

    std::vector<double> predict_next(const std::vector<double>& belief) const;

    std::vector<double> update_with_evidence(
        const std::vector<double>& predicted_belief,
        const std::vector<double>& evidence_likelihood
    ) const;

    std::vector<std::vector<double>> forward_filter(
        const std::vector<std::vector<double>>& evidence_likelihoods
    ) const;

private:
    std::vector<std::string> states_;
    std::vector<double> initial_distribution_;
    std::vector<std::vector<double>> transition_matrix_;

    void reset_defaults();

    static std::vector<double> normalise_distribution(const std::vector<double>& values);
    static void validate_square_matrix(const std::vector<std::vector<double>>& matrix, std::size_t size);
};

} // namespace ml

#endif // DYNAMIC_BAYESIAN_NETWORK_H
