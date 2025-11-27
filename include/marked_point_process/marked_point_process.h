#ifndef MARKED_POINT_PROCESS_H
#define MARKED_POINT_PROCESS_H

#include <Eigen/Dense>
#include <vector>
#include <functional>
#include <memory>

/**
 * @class MarkedPointProcess
 * @brief Implements a marked point process model for temporal event sequences with associated marks.
 * 
 * A marked point process models sequences of events that occur at specific times,
 * where each event has an associated mark (label or feature). This is useful for
 * modeling temporal patterns in data such as:
 * - User activity logs (time + action type)
 * - Financial transactions (time + transaction type/amount)
 * - Earthquake sequences (time + magnitude)
 * - Social network events (time + interaction type)
 */
class MarkedPointProcess {
public:
    /**
     * @brief Constructor for the marked point process model.
     * @param num_marks Number of distinct mark types.
     * @param learning_rate Learning rate for parameter updates.
     * @param max_iterations Maximum training iterations.
     */
    MarkedPointProcess(
        int num_marks = 2,
        double learning_rate = 0.01,
        int max_iterations = 1000
    );

    /**
     * @brief Train the model using sequences of events.
     * @param event_times Vector of event time sequences, where each sequence is a vector of timestamps.
     * @param event_marks Vector of event mark sequences, where each sequence is a vector of mark indices.
     */
    void fit(
        const std::vector<std::vector<double>>& event_times,
        const std::vector<std::vector<int>>& event_marks
    );

    /**
     * @brief Predict the conditional intensity (rate) at a given time for each mark type.
     * @param time The time point at which to evaluate the intensity.
     * @param history_times Times of previous events.
     * @param history_marks Marks of previous events.
     * @return Vector of intensities for each mark type.
     */
    Eigen::VectorXd predict_intensity(
        double time,
        const std::vector<double>& history_times,
        const std::vector<int>& history_marks
    ) const;

    /**
     * @brief Generate a sequence of events using the learned model.
     * @param time_horizon Maximum time for the generated sequence.
     * @param max_events Maximum number of events to generate.
     * @return Pair of (event_times, event_marks) for the generated sequence.
     */
    std::pair<std::vector<double>, std::vector<int>> generate_sequence(
        double time_horizon,
        int max_events = 1000
    ) const;

    /**
     * @brief Calculate the log-likelihood of observed sequences.
     * @param event_times Vector of event time sequences.
     * @param event_marks Vector of event mark sequences.
     * @return Log-likelihood value.
     */
    double log_likelihood(
        const std::vector<std::vector<double>>& event_times,
        const std::vector<std::vector<int>>& event_marks
    ) const;

    /**
     * @brief Get the base intensity (background rate) for each mark type.
     * @return Vector of base intensities.
     */
    Eigen::VectorXd get_base_intensity() const;

    /**
     * @brief Get the excitation matrix (how marks excite each other).
     * @return Matrix where entry (i,j) is the excitation from mark i to mark j.
     */
    Eigen::MatrixXd get_excitation_matrix() const;

    /**
     * @brief Get the decay parameter for the temporal kernel.
     * @return Decay rate.
     */
    double get_decay_rate() const;

    /**
     * @brief Set the base intensity parameters.
     * @param base_intensity Vector of base intensities for each mark type.
     */
    void set_base_intensity(const Eigen::VectorXd& base_intensity);

    /**
     * @brief Set the excitation matrix parameters.
     * @param excitation Matrix of excitation parameters.
     */
    void set_excitation_matrix(const Eigen::MatrixXd& excitation);

    /**
     * @brief Set the decay rate parameter.
     * @param decay Decay rate for the temporal kernel.
     */
    void set_decay_rate(double decay);

private:
    int num_marks_;
    double learning_rate_;
    int max_iterations_;

    // Model parameters
    Eigen::VectorXd mu_;        // Base intensity (background rate) for each mark
    Eigen::MatrixXd alpha_;     // Excitation matrix: alpha(i,j) = effect of mark i on mark j
    double beta_;               // Decay rate for the exponential kernel

    /**
     * @brief Initialize model parameters with random values.
     */
    void initialize_parameters();

    /**
     * @brief Compute the temporal kernel (exponential decay).
     * @param dt Time difference.
     * @return Kernel value.
     */
    double kernel(double dt) const;

    /**
     * @brief Compute the integral of the kernel from 0 to t.
     * @param t Upper limit of integration.
     * @return Integral value.
     */
    double kernel_integral(double t) const;

    /**
     * @brief Compute intensity at time t given history.
     * @param time Current time.
     * @param mark Mark type to compute intensity for.
     * @param history_times Times of previous events.
     * @param history_marks Marks of previous events.
     * @return Intensity value.
     */
    double compute_intensity(
        double time,
        int mark,
        const std::vector<double>& history_times,
        const std::vector<int>& history_marks
    ) const;

    /**
     * @brief Update parameters using gradient ascent.
     * @param event_times Training event times.
     * @param event_marks Training event marks.
     */
    void update_parameters(
        const std::vector<std::vector<double>>& event_times,
        const std::vector<std::vector<int>>& event_marks
    );

    /**
     * @brief Compute the compensator (integrated intensity) for a sequence.
     * @param event_times Event times in the sequence.
     * @param event_marks Event marks in the sequence.
     * @param time_horizon End time of observation window.
     * @return Compensator value for each mark type.
     */
    Eigen::VectorXd compute_compensator(
        const std::vector<double>& event_times,
        const std::vector<int>& event_marks,
        double time_horizon
    ) const;
};

#endif // MARKED_POINT_PROCESS_H
