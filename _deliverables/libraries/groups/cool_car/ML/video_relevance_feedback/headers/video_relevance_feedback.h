#ifndef VIDEO_RELEVANCE_FEEDBACK_H
#define VIDEO_RELEVANCE_FEEDBACK_H

#include <cstddef>
#include <string>
#include <vector>

namespace ml {
namespace video_retrieval {

struct LabeledVideo {
    std::string id;
    std::vector<double> features;
    int label = -1;
};

struct VideoItem {
    std::string id;
    std::vector<double> features;
};

struct SourceTask {
    std::string name;
    std::vector<LabeledVideo> support_vectors;
};

struct RankedVideo {
    std::string id;
    double score = 0.0;
};

enum class SourceSelectionMethod {
    ScoreAccuracy,
    ScoreClustering,
    MaxMargin
};

enum class FeedbackStrategy {
    TopRanked,
    Uncertainty,
    Random
};

struct RfTlConfig {
    double target_weight = 1.0;
    double source_weight = 0.2;
    double learning_rate = 0.05;
    double regularization = 0.01;
    std::size_t epochs = 250;
    unsigned random_seed = 13;
};

class RfTlVideoRanker {
public:
    explicit RfTlVideoRanker(RfTlConfig config = {});

    void fit(
        const std::vector<LabeledVideo>& target_feedback,
        const std::vector<SourceTask>& source_tasks,
        const std::vector<VideoItem>& database,
        SourceSelectionMethod method = SourceSelectionMethod::ScoreAccuracy);

    std::vector<RankedVideo> rank(const std::vector<VideoItem>& database) const;
    std::vector<VideoItem> solicit_feedback(
        const std::vector<VideoItem>& database,
        FeedbackStrategy strategy,
        std::size_t count) const;

    double score(const std::vector<double>& features) const;
    const std::string& selected_source_name() const;
    const std::vector<double>& weights() const;
    double bias() const;

private:
    struct LinearModel {
        std::vector<double> weights;
        double bias = 0.0;
    };

    RfTlConfig config_;
    LinearModel model_;
    std::string selected_source_name_;

    LinearModel train_weighted(
        const std::vector<LabeledVideo>& target_feedback,
        const SourceTask* source_task) const;
    std::size_t select_source(
        const std::vector<LabeledVideo>& target_feedback,
        const std::vector<SourceTask>& source_tasks,
        const std::vector<VideoItem>& database,
        SourceSelectionMethod method) const;
    double score_accuracy(const LinearModel& target_model, const SourceTask& source_task) const;
    double score_clustering(const LinearModel& model, const std::vector<VideoItem>& database) const;
    double max_margin_gap(const LinearModel& model, const std::vector<VideoItem>& database) const;
    double model_score(const LinearModel& model, const std::vector<double>& features) const;
};

std::vector<VideoItem> normalize_by_standard_deviation(const std::vector<VideoItem>& database);

} // namespace video_retrieval
} // namespace ml

#endif // VIDEO_RELEVANCE_FEEDBACK_H