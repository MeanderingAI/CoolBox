#include "time_series/time_series.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace time_series {

// TimeSeries implementation
TimeSeries::TimeSeries() {}

TimeSeries::TimeSeries(const std::vector<double>& values, 
                       const std::vector<std::string>& timestamps)
    : values_(values), timestamps_(timestamps) {}

double TimeSeries::at(size_t index) const {
    if (index >= values_.size()) {
        throw std::out_of_range("Index out of range");
    }
    return values_[index];
}

double& TimeSeries::at(size_t index) {
    if (index >= values_.size()) {
        throw std::out_of_range("Index out of range");
    }
    return values_[index];
}

std::string TimeSeries::timestamp_at(size_t index) const {
    if (index >= timestamps_.size()) {
        return "";
    }
    return timestamps_[index];
}

double TimeSeries::mean() const {
    if (values_.empty()) return 0.0;
    return std::accumulate(values_.begin(), values_.end(), 0.0) / values_.size();
}

double TimeSeries::std() const {
    if (values_.size() < 2) return 0.0;
    double m = mean();
    double sq_sum = std::accumulate(values_.begin(), values_.end(), 0.0,
        [m](double acc, double val) { return acc + (val - m) * (val - m); });
    return std::sqrt(sq_sum / (values_.size() - 1));
}

double TimeSeries::min() const {
    if (values_.empty()) return 0.0;
    return *std::min_element(values_.begin(), values_.end());
}

double TimeSeries::max() const {
    if (values_.empty()) return 0.0;
    return *std::max_element(values_.begin(), values_.end());
}

double TimeSeries::median() const {
    if (values_.empty()) return 0.0;
    std::vector<double> sorted = values_;
    std::sort(sorted.begin(), sorted.end());
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    }
    return sorted[n/2];
}

TimeSeries TimeSeries::normalize() const {
    double m = mean();
    double s = std();
    if (s == 0.0) return *this;
    
    std::vector<double> normalized;
    normalized.reserve(values_.size());
    for (double val : values_) {
        normalized.push_back((val - m) / s);
    }
    return TimeSeries(normalized, timestamps_);
}

TimeSeries TimeSeries::min_max_scale(double min_val, double max_val) const {
    double data_min = min();
    double data_max = max();
    double range = data_max - data_min;
    if (range == 0.0) return *this;
    
    std::vector<double> scaled;
    scaled.reserve(values_.size());
    for (double val : values_) {
        double normalized = (val - data_min) / range;
        scaled.push_back(normalized * (max_val - min_val) + min_val);
    }
    return TimeSeries(scaled, timestamps_);
}

TimeSeries TimeSeries::diff(size_t lag) const {
    if (values_.size() <= lag) {
        return TimeSeries();
    }
    
    std::vector<double> differenced;
    differenced.reserve(values_.size() - lag);
    for (size_t i = lag; i < values_.size(); ++i) {
        differenced.push_back(values_[i] - values_[i - lag]);
    }
    
    std::vector<std::string> new_timestamps;
    if (!timestamps_.empty() && timestamps_.size() > lag) {
        new_timestamps.assign(timestamps_.begin() + lag, timestamps_.end());
    }
    
    return TimeSeries(differenced, new_timestamps);
}

TimeSeries TimeSeries::log_transform() const {
    std::vector<double> transformed;
    transformed.reserve(values_.size());
    for (double val : values_) {
        if (val <= 0.0) {
            throw std::runtime_error("Log transform requires positive values");
        }
        transformed.push_back(std::log(val));
    }
    return TimeSeries(transformed, timestamps_);
}

TimeSeries TimeSeries::moving_average(size_t window_size) const {
    if (window_size == 0 || window_size > values_.size()) {
        return *this;
    }
    
    std::vector<double> smoothed;
    smoothed.reserve(values_.size());
    
    for (size_t i = 0; i < values_.size(); ++i) {
        size_t start = (i >= window_size - 1) ? i - window_size + 1 : 0;
        size_t count = i - start + 1;
        
        double sum = 0.0;
        for (size_t j = start; j <= i; ++j) {
            sum += values_[j];
        }
        smoothed.push_back(sum / count);
    }
    
    return TimeSeries(smoothed, timestamps_);
}

TimeSeries TimeSeries::exponential_smoothing(double alpha) const {
    if (values_.empty()) return *this;
    if (alpha < 0.0 || alpha > 1.0) {
        throw std::invalid_argument("Alpha must be between 0 and 1");
    }
    
    std::vector<double> smoothed;
    smoothed.reserve(values_.size());
    smoothed.push_back(values_[0]);
    
    for (size_t i = 1; i < values_.size(); ++i) {
        double s = alpha * values_[i] + (1.0 - alpha) * smoothed[i - 1];
        smoothed.push_back(s);
    }
    
    return TimeSeries(smoothed, timestamps_);
}

TimeSeries TimeSeries::resample(size_t new_size) const {
    if (new_size == 0 || values_.empty()) {
        return TimeSeries();
    }
    
    std::vector<double> resampled;
    resampled.reserve(new_size);
    
    double ratio = static_cast<double>(values_.size() - 1) / (new_size - 1);
    
    for (size_t i = 0; i < new_size; ++i) {
        double idx = i * ratio;
        size_t lower = static_cast<size_t>(idx);
        size_t upper = std::min(lower + 1, values_.size() - 1);
        double weight = idx - lower;
        
        double value = values_[lower] * (1.0 - weight) + values_[upper] * weight;
        resampled.push_back(value);
    }
    
    return TimeSeries(resampled);
}

std::vector<std::vector<double>> TimeSeries::create_windows(size_t window_size, size_t stride) const {
    std::vector<std::vector<double>> windows;
    
    if (window_size > values_.size()) {
        return windows;
    }
    
    for (size_t i = 0; i + window_size <= values_.size(); i += stride) {
        std::vector<double> window(values_.begin() + i, values_.begin() + i + window_size);
        windows.push_back(window);
    }
    
    return windows;
}

std::pair<std::vector<std::vector<double>>, std::vector<double>> 
TimeSeries::create_supervised_windows(size_t input_window, size_t output_window, size_t stride) const {
    std::vector<std::vector<double>> X;
    std::vector<double> y;
    
    size_t total_window = input_window + output_window;
    if (total_window > values_.size()) {
        return {X, y};
    }
    
    for (size_t i = 0; i + total_window <= values_.size(); i += stride) {
        std::vector<double> input(values_.begin() + i, values_.begin() + i + input_window);
        X.push_back(input);
        
        if (output_window == 1) {
            y.push_back(values_[i + input_window]);
        } else {
            // For multi-step, use last value
            y.push_back(values_[i + input_window + output_window - 1]);
        }
    }
    
    return {X, y};
}

std::vector<double> TimeSeries::autocorrelation(size_t max_lag) const {
    if (values_.empty()) return {};
    
    double m = mean();
    double variance = 0.0;
    for (double val : values_) {
        variance += (val - m) * (val - m);
    }
    
    std::vector<double> acf;
    acf.reserve(max_lag + 1);
    
    for (size_t lag = 0; lag <= max_lag && lag < values_.size(); ++lag) {
        double covariance = 0.0;
        for (size_t i = lag; i < values_.size(); ++i) {
            covariance += (values_[i] - m) * (values_[i - lag] - m);
        }
        acf.push_back(covariance / variance);
    }
    
    return acf;
}

// MultivariateTimeSeries implementation
MultivariatTimeSeries::MultivariatTimeSeries() {}

MultivariatTimeSeries::MultivariatTimeSeries(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& feature_names,
    const std::vector<std::string>& timestamps)
    : data_(data), feature_names_(feature_names), timestamps_(timestamps) {}

const std::vector<double>& MultivariatTimeSeries::feature(size_t index) const {
    if (index >= data_.size()) {
        throw std::out_of_range("Feature index out of range");
    }
    return data_[index];
}

std::vector<double> MultivariatTimeSeries::sample(size_t index) const {
    std::vector<double> result;
    result.reserve(data_.size());
    for (const auto& feature : data_) {
        if (index >= feature.size()) {
            throw std::out_of_range("Sample index out of range");
        }
        result.push_back(feature[index]);
    }
    return result;
}

double MultivariatTimeSeries::at(size_t feature_idx, size_t sample_idx) const {
    if (feature_idx >= data_.size()) {
        throw std::out_of_range("Feature index out of range");
    }
    if (sample_idx >= data_[feature_idx].size()) {
        throw std::out_of_range("Sample index out of range");
    }
    return data_[feature_idx][sample_idx];
}

std::vector<double> MultivariatTimeSeries::means() const {
    std::vector<double> result;
    result.reserve(data_.size());
    for (const auto& feature : data_) {
        TimeSeries ts(feature);
        result.push_back(ts.mean());
    }
    return result;
}

std::vector<double> MultivariatTimeSeries::stds() const {
    std::vector<double> result;
    result.reserve(data_.size());
    for (const auto& feature : data_) {
        TimeSeries ts(feature);
        result.push_back(ts.std());
    }
    return result;
}

MultivariatTimeSeries MultivariatTimeSeries::normalize() const {
    std::vector<std::vector<double>> normalized_data;
    normalized_data.reserve(data_.size());
    
    for (const auto& feature : data_) {
        TimeSeries ts(feature);
        normalized_data.push_back(ts.normalize().values());
    }
    
    return MultivariatTimeSeries(normalized_data, feature_names_, timestamps_);
}

MultivariatTimeSeries MultivariatTimeSeries::min_max_scale() const {
    std::vector<std::vector<double>> scaled_data;
    scaled_data.reserve(data_.size());
    
    for (const auto& feature : data_) {
        TimeSeries ts(feature);
        scaled_data.push_back(ts.min_max_scale().values());
    }
    
    return MultivariatTimeSeries(scaled_data, feature_names_, timestamps_);
}

std::vector<std::vector<std::vector<double>>> 
MultivariatTimeSeries::create_windows(size_t window_size, size_t stride) const {
    std::vector<std::vector<std::vector<double>>> windows;
    
    if (data_.empty() || data_[0].size() < window_size) {
        return windows;
    }
    
    size_t num_windows = (data_[0].size() - window_size) / stride + 1;
    windows.reserve(num_windows);
    
    for (size_t i = 0; i + window_size <= data_[0].size(); i += stride) {
        std::vector<std::vector<double>> window;
        window.reserve(data_.size());
        
        for (const auto& feature : data_) {
            std::vector<double> feature_window(feature.begin() + i, feature.begin() + i + window_size);
            window.push_back(feature_window);
        }
        windows.push_back(window);
    }
    
    return windows;
}

// Forecasting models

MovingAverageForecaster::MovingAverageForecaster(size_t window_size)
    : window_size_(window_size) {}

void MovingAverageForecaster::fit(const TimeSeries& ts) {
    const auto& values = ts.values();
    size_t start = values.size() >= window_size_ ? values.size() - window_size_ : 0;
    last_values_.assign(values.begin() + start, values.end());
}

std::vector<double> MovingAverageForecaster::forecast(size_t steps) const {
    std::vector<double> predictions;
    predictions.reserve(steps);
    
    std::vector<double> buffer = last_values_;
    
    for (size_t i = 0; i < steps; ++i) {
        double prediction = std::accumulate(buffer.end() - std::min(window_size_, buffer.size()), 
                                           buffer.end(), 0.0) / std::min(window_size_, buffer.size());
        predictions.push_back(prediction);
        buffer.push_back(prediction);
    }
    
    return predictions;
}

double MovingAverageForecaster::forecast_one_step() const {
    if (last_values_.empty()) return 0.0;
    size_t count = std::min(window_size_, last_values_.size());
    return std::accumulate(last_values_.end() - count, last_values_.end(), 0.0) / count;
}

// Exponential Smoothing
ExponentialSmoothingForecaster::ExponentialSmoothingForecaster(
    double alpha, double beta, double gamma)
    : alpha_(alpha), beta_(beta), gamma_(gamma), level_(0.0), trend_(0.0) {}

void ExponentialSmoothingForecaster::fit(const TimeSeries& ts) {
    const auto& values = ts.values();
    if (values.empty()) return;
    
    level_ = values[0];
    trend_ = values.size() > 1 ? values[1] - values[0] : 0.0;
    
    for (size_t i = 1; i < values.size(); ++i) {
        double old_level = level_;
        level_ = alpha_ * values[i] + (1.0 - alpha_) * (level_ + trend_);
        trend_ = beta_ * (level_ - old_level) + (1.0 - beta_) * trend_;
    }
}

std::vector<double> ExponentialSmoothingForecaster::forecast(size_t steps) const {
    std::vector<double> predictions;
    predictions.reserve(steps);
    
    for (size_t i = 1; i <= steps; ++i) {
        predictions.push_back(level_ + i * trend_);
    }
    
    return predictions;
}

// Auto-regressive Model
AutoRegressiveModel::AutoRegressiveModel(size_t order)
    : order_(order) {}

void AutoRegressiveModel::fit(const TimeSeries& ts) {
    const auto& values = ts.values();
    if (values.size() <= order_) {
        throw std::runtime_error("Not enough data for AR model");
    }
    
    // Simple least squares estimation
    size_t n = values.size() - order_;
    std::vector<std::vector<double>> X(n, std::vector<double>(order_));
    std::vector<double> y(n);
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < order_; ++j) {
            X[i][j] = values[order_ - 1 - j + i];
        }
        y[i] = values[order_ + i];
    }
    
    // Solve normal equations (simplified - using naive approach)
    coefficients_.resize(order_, 0.0);
    
    for (size_t j = 0; j < order_; ++j) {
        double num = 0.0, denom = 0.0;
        for (size_t i = 0; i < n; ++i) {
            num += X[i][j] * y[i];
            denom += X[i][j] * X[i][j];
        }
        coefficients_[j] = (denom != 0.0) ? num / denom : 0.0;
    }
    
    // Store last values for forecasting
    last_values_.assign(values.end() - order_, values.end());
}

std::vector<double> AutoRegressiveModel::forecast(size_t steps) const {
    std::vector<double> predictions;
    predictions.reserve(steps);
    
    std::vector<double> buffer = last_values_;
    
    for (size_t i = 0; i < steps; ++i) {
        double pred = 0.0;
        for (size_t j = 0; j < order_; ++j) {
            pred += coefficients_[j] * buffer[buffer.size() - 1 - j];
        }
        predictions.push_back(pred);
        buffer.push_back(pred);
    }
    
    return predictions;
}

// Utility functions
SeasonalDecomposition seasonal_decompose(const TimeSeries& ts, size_t period) {
    SeasonalDecomposition result;
    
    // Extract trend using moving average
    result.trend = ts.moving_average(period);
    
    // Detrend
    const auto& values = ts.values();
    const auto& trend_values = result.trend.values();
    
    std::vector<double> detrended(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        detrended[i] = values[i] - (i < trend_values.size() ? trend_values[i] : trend_values.back());
    }
    
    // Extract seasonal component
    std::vector<double> seasonal_pattern(period, 0.0);
    std::vector<size_t> counts(period, 0);
    
    for (size_t i = 0; i < detrended.size(); ++i) {
        size_t idx = i % period;
        seasonal_pattern[idx] += detrended[i];
        counts[idx]++;
    }
    
    for (size_t i = 0; i < period; ++i) {
        if (counts[i] > 0) {
            seasonal_pattern[i] /= counts[i];
        }
    }
    
    std::vector<double> seasonal_values;
    seasonal_values.reserve(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        seasonal_values.push_back(seasonal_pattern[i % period]);
    }
    result.seasonal = TimeSeries(seasonal_values);
    
    // Residual
    std::vector<double> residual_values(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        double trend_val = i < trend_values.size() ? trend_values[i] : trend_values.back();
        residual_values[i] = values[i] - trend_val - seasonal_values[i];
    }
    result.residual = TimeSeries(residual_values);
    
    return result;
}

std::vector<double> detect_outliers_zscore(const TimeSeries& ts, double threshold) {
    std::vector<double> outlier_indices;
    double m = ts.mean();
    double s = ts.std();
    
    if (s == 0.0) return outlier_indices;
    
    const auto& values = ts.values();
    for (size_t i = 0; i < values.size(); ++i) {
        double z = std::abs((values[i] - m) / s);
        if (z > threshold) {
            outlier_indices.push_back(static_cast<double>(i));
        }
    }
    
    return outlier_indices;
}

std::vector<double> detect_outliers_iqr(const TimeSeries& ts, double multiplier) {
    std::vector<double> outlier_indices;
    
    const auto& values = ts.values();
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    size_t n = sorted.size();
    if (n < 4) return outlier_indices;
    
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;
    
    double lower = q1 - multiplier * iqr;
    double upper = q3 + multiplier * iqr;
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < lower || values[i] > upper) {
            outlier_indices.push_back(static_cast<double>(i));
        }
    }
    
    return outlier_indices;
}

TimeSeries interpolate_missing(const TimeSeries& ts, const std::vector<size_t>& missing_indices) {
    std::vector<double> interpolated = ts.values();
    
    for (size_t idx : missing_indices) {
        if (idx >= interpolated.size()) continue;
        
        // Linear interpolation
        size_t left = idx;
        while (left > 0 && std::find(missing_indices.begin(), missing_indices.end(), left) != missing_indices.end()) {
            left--;
        }
        
        size_t right = idx;
        while (right < interpolated.size() - 1 && 
               std::find(missing_indices.begin(), missing_indices.end(), right) != missing_indices.end()) {
            right++;
        }
        
        if (left < idx && right > idx) {
            double weight = static_cast<double>(idx - left) / (right - left);
            interpolated[idx] = interpolated[left] * (1.0 - weight) + interpolated[right] * weight;
        } else if (left < idx) {
            interpolated[idx] = interpolated[left];
        } else if (right > idx) {
            interpolated[idx] = interpolated[right];
        }
    }
    
    return TimeSeries(interpolated, ts.timestamp_at(0) != "" ? 
        std::vector<std::string>(ts.size(), "") : std::vector<std::string>());
}

} // namespace time_series
} // namespace ml
