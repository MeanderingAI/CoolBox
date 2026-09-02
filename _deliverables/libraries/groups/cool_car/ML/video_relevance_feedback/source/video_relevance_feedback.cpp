#include "video_relevance_feedback.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace {

int normalized_label(int label) {
    return label > 0 ? 1 : -1;
}

std::size_t feature_count_from_feedback(const std::vector<ml::video_retrieval::LabeledVideo>& videos) {
    if (videos.empty()) return 0;
    return videos.front().features.size();
}

void validate_feature_size(const std::vector<double>& features, std::size_t expected) {
    if (features.size() != expected) throw std::invalid_argument("Video feature vector length mismatch");
}

double dot_product(const std::vector<double>& left, const std::vector<double>& right) {
    double value = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) value += left[index] * right[index];
    return value;
}

} // namespace

namespace ml {
namespace video_retrieval {

RfTlVideoRanker::RfTlVideoRanker(RfTlConfig config) : config_(config) {}

void RfTlVideoRanker::fit(
    const std::vector<LabeledVideo>& target_feedback,
    const std::vector<SourceTask>& source_tasks,
    const std::vector<VideoItem>& database,
    SourceSelectionMethod method) {
    if (target_feedback.empty()) throw std::invalid_argument("RF/TL ranking requires target feedback");
    if (database.empty()) throw std::invalid_argument("RF/TL ranking requires a database to rank");

    const SourceTask* selected_source = nullptr;
    selected_source_name_.clear();
    if (!source_tasks.empty()) {
        const std::size_t selected = select_source(target_feedback, source_tasks, database, method);
        selected_source = &source_tasks[selected];
        selected_source_name_ = selected_source->name;
    }
    model_ = train_weighted(target_feedback, selected_source);
}

std::vector<RankedVideo> RfTlVideoRanker::rank(const std::vector<VideoItem>& database) const {
    std::vector<RankedVideo> ranked;
    ranked.reserve(database.size());
    for (const auto& item : database) ranked.push_back({item.id, score(item.features)});
    std::stable_sort(ranked.begin(), ranked.end(), [](const RankedVideo& left, const RankedVideo& right) {
        return left.score > right.score;
    });
    return ranked;
}

std::vector<VideoItem> RfTlVideoRanker::solicit_feedback(
    const std::vector<VideoItem>& database,
    FeedbackStrategy strategy,
    std::size_t count) const {
    std::vector<VideoItem> candidates = database;
    if (strategy == FeedbackStrategy::TopRanked) {
        std::stable_sort(candidates.begin(), candidates.end(), [this](const VideoItem& left, const VideoItem& right) {
            return score(left.features) > score(right.features);
        });
    } else if (strategy == FeedbackStrategy::Uncertainty) {
        std::stable_sort(candidates.begin(), candidates.end(), [this](const VideoItem& left, const VideoItem& right) {
            return std::abs(score(left.features)) < std::abs(score(right.features));
        });
    } else {
        std::mt19937 rng(config_.random_seed);
        std::shuffle(candidates.begin(), candidates.end(), rng);
    }
    if (candidates.size() > count) candidates.resize(count);
    return candidates;
}

double RfTlVideoRanker::score(const std::vector<double>& features) const {
    return model_score(model_, features);
}

const std::string& RfTlVideoRanker::selected_source_name() const { return selected_source_name_; }

const std::vector<double>& RfTlVideoRanker::weights() const { return model_.weights; }

double RfTlVideoRanker::bias() const { return model_.bias; }

RfTlVideoRanker::LinearModel RfTlVideoRanker::train_weighted(
    const std::vector<LabeledVideo>& target_feedback,
    const SourceTask* source_task) const {
    const std::size_t feature_count = feature_count_from_feedback(target_feedback);
    if (feature_count == 0) throw std::invalid_argument("Video feature vectors cannot be empty");

    struct TrainingRow {
        std::vector<double> features;
        int label = -1;
        double weight = 1.0;
    };

    std::vector<TrainingRow> rows;
    for (const auto& video : target_feedback) {
        validate_feature_size(video.features, feature_count);
        rows.push_back({video.features, normalized_label(video.label), config_.target_weight});
    }
    if (source_task != nullptr) {
        for (const auto& video : source_task->support_vectors) {
            validate_feature_size(video.features, feature_count);
            rows.push_back({video.features, normalized_label(video.label), config_.source_weight});
        }
    }

    LinearModel model;
    model.weights.assign(feature_count, 0.0);
    for (std::size_t epoch = 0; epoch < config_.epochs; ++epoch) {
        const double step = config_.learning_rate / (1.0 + 0.02 * static_cast<double>(epoch));
        for (const auto& row : rows) {
            for (double& weight : model.weights) weight *= (1.0 - step * config_.regularization);
            const double margin = static_cast<double>(row.label) * model_score(model, row.features);
            if (margin < 1.0) {
                for (std::size_t index = 0; index < feature_count; ++index) {
                    model.weights[index] += step * row.weight * static_cast<double>(row.label) * row.features[index];
                }
                model.bias += step * row.weight * static_cast<double>(row.label);
            }
        }
    }
    return model;
}

std::size_t RfTlVideoRanker::select_source(
    const std::vector<LabeledVideo>& target_feedback,
    const std::vector<SourceTask>& source_tasks,
    const std::vector<VideoItem>& database,
    SourceSelectionMethod method) const {
    double best_score = -std::numeric_limits<double>::infinity();
    std::size_t best_index = 0;
    const LinearModel target_model = train_weighted(target_feedback, nullptr);
    for (std::size_t index = 0; index < source_tasks.size(); ++index) {
        double candidate = 0.0;
        if (method == SourceSelectionMethod::ScoreAccuracy) {
            candidate = score_accuracy(target_model, source_tasks[index]);
        } else {
            const LinearModel transfer_model = train_weighted(target_feedback, &source_tasks[index]);
            candidate = method == SourceSelectionMethod::ScoreClustering
                ? score_clustering(transfer_model, database)
                : max_margin_gap(transfer_model, database);
        }
        if (candidate > best_score) {
            best_score = candidate;
            best_index = index;
        }
    }
    return best_index;
}

double RfTlVideoRanker::score_accuracy(const LinearModel& target_model, const SourceTask& source_task) const {
    if (source_task.support_vectors.empty()) return 0.0;
    double correct = 0.0;
    for (const auto& video : source_task.support_vectors) {
        const int prediction = model_score(target_model, video.features) >= 0.0 ? 1 : -1;
        if (prediction == normalized_label(video.label)) correct += 1.0;
    }
    return correct / static_cast<double>(source_task.support_vectors.size());
}

double RfTlVideoRanker::score_clustering(const LinearModel& model, const std::vector<VideoItem>& database) const {
    if (database.size() < 2) return 0.0;
    std::vector<double> scores;
    scores.reserve(database.size());
    for (const auto& item : database) scores.push_back(model_score(model, item.features));
    auto minmax = std::minmax_element(scores.begin(), scores.end());
    double low_mean = *minmax.first;
    double high_mean = *minmax.second;
    for (int iteration = 0; iteration < 12; ++iteration) {
        double low_sum = 0.0;
        double high_sum = 0.0;
        std::size_t low_count = 0;
        std::size_t high_count = 0;
        for (double value : scores) {
            if (std::abs(value - low_mean) <= std::abs(value - high_mean)) {
                low_sum += value;
                ++low_count;
            } else {
                high_sum += value;
                ++high_count;
            }
        }
        if (low_count > 0) low_mean = low_sum / static_cast<double>(low_count);
        if (high_count > 0) high_mean = high_sum / static_cast<double>(high_count);
    }
    const double separation = high_mean - low_mean;
    return separation * separation;
}

double RfTlVideoRanker::max_margin_gap(const LinearModel& model, const std::vector<VideoItem>& database) const {
    double nearest_positive = std::numeric_limits<double>::infinity();
    double nearest_negative = -std::numeric_limits<double>::infinity();
    for (const auto& item : database) {
        const double value = model_score(model, item.features);
        if (value >= 0.0) nearest_positive = std::min(nearest_positive, value);
        else nearest_negative = std::max(nearest_negative, value);
    }
    if (!std::isfinite(nearest_positive) || !std::isfinite(nearest_negative)) return 0.0;
    return nearest_positive - nearest_negative;
}

double RfTlVideoRanker::model_score(const LinearModel& model, const std::vector<double>& features) const {
    validate_feature_size(features, model.weights.size());
    return dot_product(model.weights, features) + model.bias;
}

std::vector<VideoItem> normalize_by_standard_deviation(const std::vector<VideoItem>& database) {
    if (database.empty()) return {};
    const std::size_t feature_count = database.front().features.size();
    std::vector<double> means(feature_count, 0.0);
    for (const auto& item : database) {
        validate_feature_size(item.features, feature_count);
        for (std::size_t index = 0; index < feature_count; ++index) means[index] += item.features[index];
    }
    for (double& mean : means) mean /= static_cast<double>(database.size());

    std::vector<double> variances(feature_count, 0.0);
    for (const auto& item : database) {
        for (std::size_t index = 0; index < feature_count; ++index) {
            const double centered = item.features[index] - means[index];
            variances[index] += centered * centered;
        }
    }

    std::vector<VideoItem> normalized = database;
    for (auto& item : normalized) {
        for (std::size_t index = 0; index < feature_count; ++index) {
            const double stddev = std::sqrt(variances[index] / static_cast<double>(database.size()));
            item.features[index] = stddev > 1.0e-12 ? item.features[index] / stddev : item.features[index];
        }
    }
    return normalized;
}

} // namespace video_retrieval
} // namespace ml