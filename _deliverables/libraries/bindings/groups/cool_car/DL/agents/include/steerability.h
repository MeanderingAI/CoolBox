#ifndef ML_DEEP_LEARNING_AGENTS_STEERABILITY_H
#define ML_DEEP_LEARNING_AGENTS_STEERABILITY_H

#include <cstddef>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// @brief Goal-space steerability evaluation.
///
/// Implements Chang et al., "A Course Correction in Steerability Evaluation:
/// Revealing Miscalibration and Side Effects in LLMs", AAAI 2026
/// (arXiv:2505.23816), citation key @c chang_steerability_2025.
///
/// Source text, user goal and model output are all represented as vectors in a
/// multi-dimensional goal space whose axes are normalised text attributes such
/// as reading difficulty or formality. Scalar win rates hide two distinct
/// failure modes that this decomposition separates: @em miscalibration, moving
/// the wrong distance along the axis the user asked about, and @em side
/// @em effects, moving along axes the user never mentioned.

using GoalVector = std::vector<double>;

/// One rewriting episode: the model was asked to move @c source to @c goal and
/// actually produced @c output.
struct SteeringSample {
    GoalVector source;
    GoalVector goal;
    GoalVector output;
};

struct SteeringDecomposition {
    /// ||goal - source||, how far the user asked the text to move.
    double requested_distance = 0.0;
    /// Component of the realised movement along the requested direction.
    double achieved_along_goal = 0.0;
    /// achieved/requested - 1. Negative means undershooting the request.
    double miscalibration = 0.0;
    /// Magnitude of the movement orthogonal to the request.
    double side_effect_magnitude = 0.0;
    /// ||output - goal||, the residual the user actually experiences.
    double steering_error = 0.0;
    /// Per-axis orthogonal movement, for attributing side effects to attributes.
    GoalVector per_dimension_side_effect;
};

SteeringDecomposition decompose(const SteeringSample& sample);

struct SteerabilityReport {
    size_t samples = 0;
    double mean_steering_error = 0.0;
    double mean_miscalibration = 0.0;
    double mean_side_effect = 0.0;
    /// 1 - E||output - goal|| / E||source - goal||: how much closer to the goal
    /// the model got than leaving the text untouched. Zero means useless, one
    /// means perfect, negative means actively counterproductive.
    double steerability_index = 0.0;
    /// Fraction of episodes that moved less far than requested.
    double undershoot_rate = 0.0;
    GoalVector mean_per_dimension_side_effect;
};

SteerabilityReport evaluate_steerability(const std::vector<SteeringSample>& samples);

/// Axis that absorbed the most unrequested movement across the samples.
size_t worst_side_effect_dimension(const SteerabilityReport& report);

// ----------------------------------------------------------------------------
// Goal sampling and interventions
// ----------------------------------------------------------------------------

/// Samples goals uniformly from the goal space rather than from scraped chats,
/// rejecting any that sit closer than @p min_distance to the source so the
/// probes actually demand a change.
std::vector<GoalVector> sample_goals(const GoalVector& source,
                                     size_t count,
                                     double min_distance,
                                     unsigned int seed);

/// Best-of-N: index of the candidate closest to the goal.
size_t best_of_n(const std::vector<GoalVector>& candidates, const GoalVector& goal);

/// A deterministic stand-in for an LLM rewriter, used to exercise the metrics.
///
/// It moves the text a @c gain fraction of the way along the requested
/// direction, which produces miscalibration when the gain is not one, and
/// leaks @c side_effect_strength of that movement into an unrequested axis.
struct SteeringSimulator {
    double gain = 1.0;
    double side_effect_strength = 0.0;
    /// Axis the leakage lands on.
    size_t side_effect_dimension = 0;

    GoalVector respond(const GoalVector& source, const GoalVector& goal) const;
};

/// Euclidean distance, with goal-space vectors clamped to [0, 1].
double goal_distance(const GoalVector& a, const GoalVector& b);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_STEERABILITY_H
