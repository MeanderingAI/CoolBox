#ifndef PIECEWISE_CONDITIONAL_INTENSITY_MODEL_H
#define PIECEWISE_CONDITIONAL_INTENSITY_MODEL_H

#include <Eigen/Dense>
#include <vector>
#include <functional>
#include <memory>
#include <map>

/**
 * @class PiecewiseConditionalIntensityModel
 * @brief Implements a piecewise conditional intensity model for temporal point processes.
 * 
 * This model divides the time domain into intervals and fits different intensity functions
 * in each interval, allowing for non-stationary behavior and regime changes. Each piece
 * can have its own conditional intensity function that depends on the history of events.
 * 
 * Applications include:
 * - Financial market microstructure (high-frequency trading)
 * - Crime modeling with time-varying patterns
 * - Healthcare monitoring with circadian rhythms
 * - Network traffic analysis with daily/weekly patterns
 * - Earthquake aftershock sequences
 */
class PiecewiseConditionalIntensityModel {
public:
    /**
     * @brief Enum for different intensity function types
     */
    enum class IntensityType {
        CONSTANT,           // Constant intensity in each piece
        LINEAR,             // Linear intensity: a + b*t
        EXPONENTIAL,        // Exponential decay: a*exp(-b*t)
        HAWKES,            // Self-exciting Hawkes process
        COX                // Cox process with covariates
    };

    /**
     * @brief Structure to define a time interval piece
     */
    struct TimeInterval {
        double start_time;
        double end_time;
        IntensityType intensity_type;
        Eigen::VectorXd parameters;  // Parameters for the intensity function
        
        TimeInterval(double start, double end, IntensityType type)
            : start_time(start), end_time(end), intensity_type(type) {}
    };

    /**
     * @brief Constructor for PCIM.
     * @param num_intervals Number of time intervals to divide the domain.
     * @param learning_rate Learning rate for parameter optimization.
     * @param max_iterations Maximum training iterations.
     */
    PiecewiseConditionalIntensityModel(
        int num_intervals = 10,
        double learning_rate = 0.01,
        int max_iterations = 1000
    );

    /**
     * @brief Define custom time intervals with specific intensity types.
     * @param intervals Vector of TimeInterval specifications.
     */
    void set_intervals(const std::vector<TimeInterval>& intervals);

    /**
     * @brief Automatically define intervals based on data with uniform width.
     * @param time_min Minimum time in the data.
     * @param time_max Maximum time in the data.
     * @param intensity_type Type of intensity function for all intervals.
     */
    void create_uniform_intervals(
        double time_min,
        double time_max,
        IntensityType intensity_type = IntensityType::HAWKES
    );

    /**
     * @brief Automatically define intervals with adaptive boundaries based on event density.
     * @param event_times All event times in the dataset.
     * @param intensity_type Type of intensity function for all intervals.
     */
    void create_adaptive_intervals(
        const std::vector<double>& event_times,
        IntensityType intensity_type = IntensityType::HAWKES
    );

    /**
     * @brief Train the model using observed event sequences.
     * @param event_times Vector of event time sequences.
     */
    void fit(const std::vector<std::vector<double>>& event_times);

    /**
     * @brief Fit with covariate information for Cox-type models.
     * @param event_times Vector of event time sequences.
     * @param covariates Matrix of covariates (rows: events, cols: covariates).
     */
    void fit_with_covariates(
        const std::vector<std::vector<double>>& event_times,
        const std::vector<Eigen::MatrixXd>& covariates
    );

    /**
     * @brief Predict the conditional intensity at a given time.
     * @param time The time point at which to evaluate intensity.
     * @param history_times Times of previous events in the current sequence.
     * @return Predicted intensity value.
     */
    double predict_intensity(
        double time,
        const std::vector<double>& history_times
    ) const;

    /**
     * @brief Predict intensity with covariate information.
     * @param time The time point.
     * @param history_times Previous event times.
     * @param covariates Covariate values at the prediction time.
     * @return Predicted intensity value.
     */
    double predict_intensity_with_covariates(
        double time,
        const std::vector<double>& history_times,
        const Eigen::VectorXd& covariates
    ) const;

    /**
     * @brief Generate a synthetic event sequence using the learned model.
     * @param time_horizon Maximum time for the generated sequence.
     * @param max_events Maximum number of events to generate.
     * @return Vector of generated event times.
     */
    std::vector<double> generate_sequence(
        double time_horizon,
        int max_events = 1000
    ) const;

    /**
     * @brief Calculate the log-likelihood of observed sequences.
     * @param event_times Vector of event time sequences.
     * @return Log-likelihood value.
     */
    double log_likelihood(const std::vector<std::vector<double>>& event_times) const;

    /**
     * @brief Get the time intervals used by the model.
     * @return Vector of time intervals.
     */
    std::vector<TimeInterval> get_intervals() const;

    /**
     * @brief Get parameters for a specific interval.
     * @param interval_index Index of the interval.
     * @return Parameter vector for that interval.
     */
    Eigen::VectorXd get_interval_parameters(int interval_index) const;

    /**
     * @brief Set parameters for a specific interval.
     * @param interval_index Index of the interval.
     * @param parameters Parameter vector to set.
     */
    void set_interval_parameters(int interval_index, const Eigen::VectorXd& parameters);

    /**
     * @brief Get the expected number of events in each interval.
     * @param event_times Event sequences to analyze.
     * @return Vector of expected counts per interval.
     */
    Eigen::VectorXd get_expected_counts(
        const std::vector<std::vector<double>>& event_times
    ) const;

    /**
     * @brief Perform model selection using information criteria.
     * @param event_times Training data.
     * @return Pair of (AIC, BIC) values.
     */
    std::pair<double, double> compute_information_criteria(
        const std::vector<std::vector<double>>& event_times
    ) const;

private:
    int num_intervals_;
    double learning_rate_;
    int max_iterations_;

    std::vector<TimeInterval> intervals_;
    std::map<int, Eigen::VectorXd> interval_parameters_;

    // Helper methods for different intensity types
    double compute_constant_intensity(
        double time,
        const Eigen::VectorXd& params,
        const std::vector<double>& history_times
    ) const;

    double compute_linear_intensity(
        double time,
        const Eigen::VectorXd& params,
        const std::vector<double>& history_times
    ) const;

    double compute_exponential_intensity(
        double time,
        const Eigen::VectorXd& params,
        const std::vector<double>& history_times
    ) const;

    double compute_hawkes_intensity(
        double time,
        const Eigen::VectorXd& params,
        const std::vector<double>& history_times
    ) const;

    double compute_cox_intensity(
        double time,
        const Eigen::VectorXd& params,
        const std::vector<double>& history_times,
        const Eigen::VectorXd& covariates
    ) const;

    /**
     * @brief Find which interval a given time belongs to.
     * @param time The time point.
     * @return Index of the interval, or -1 if not found.
     */
    int find_interval(double time) const;

    /**
     * @brief Initialize parameters for all intervals.
     */
    void initialize_parameters();

    /**
     * @brief Compute the compensator (integrated intensity) for an interval.
     * @param interval_idx Index of the interval.
     * @param event_times Event times in that interval.
     * @param all_history All events before this interval.
     * @return Compensator value.
     */
    double compute_interval_compensator(
        int interval_idx,
        const std::vector<double>& event_times,
        const std::vector<double>& all_history
    ) const;

    /**
     * @brief Update parameters for a specific interval using gradient ascent.
     * @param interval_idx Index of the interval.
     * @param event_times Events in this interval.
     * @param all_history Events before this interval.
     */
    void update_interval_parameters(
        int interval_idx,
        const std::vector<double>& event_times,
        const std::vector<double>& all_history
    );

    /**
     * @brief Compute gradient for different intensity types.
     */
    Eigen::VectorXd compute_gradient(
        int interval_idx,
        const std::vector<double>& event_times,
        const std::vector<double>& all_history
    ) const;

    /**
     * @brief Get the number of parameters for a given intensity type.
     * @param type The intensity type.
     * @return Number of parameters.
     */
    int get_num_parameters(IntensityType type) const;
};

#endif // PIECEWISE_CONDITIONAL_INTENSITY_MODEL_H
