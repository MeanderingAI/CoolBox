#ifndef CONTINUOUS_TIME_BAYESIAN_NETWORK_H
#define CONTINUOUS_TIME_BAYESIAN_NETWORK_H

#include <cstddef>
#include <string>
#include <vector>

namespace ml {

class ContinuousTimeBayesianNetwork {
public:
    ContinuousTimeBayesianNetwork();

    void set_states(const std::vector<std::string>& states);
    const std::vector<std::string>& get_states() const;

    void set_intensity_matrix(const std::vector<std::vector<double>>& intensity_matrix);
    const std::vector<std::vector<double>>& get_intensity_matrix() const;

    std::vector<std::vector<double>> transition_matrix(double dt, std::size_t series_terms = 24) const;

    std::vector<double> propagate(
        const std::vector<double>& belief,
        double dt,
        std::size_t series_terms = 24
    ) const;

    std::vector<std::vector<double>> propagate_trajectory(
        const std::vector<double>& initial_belief,
        const std::vector<double>& step_durations,
        std::size_t series_terms = 24
    ) const;

private:
    std::vector<std::string> states_;
    std::vector<std::vector<double>> intensity_matrix_;

    void reset_defaults();

    static std::vector<double> normalise_distribution(const std::vector<double>& values);
    static void validate_square_matrix(const std::vector<std::vector<double>>& matrix, std::size_t size);
};

} // namespace ml

#endif // CONTINUOUS_TIME_BAYESIAN_NETWORK_H
