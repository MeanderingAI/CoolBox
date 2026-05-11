#include "time_series.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace time_series {

// TimeSeries
TimeSeries::TimeSeries() {}

TimeSeries::TimeSeries(const std::vector<double>& values, const std::vector<std::string>& timestamps)
    : values_(values), timestamps_(timestamps) {}

double TimeSeries::at(size_t index) const { return values_.at(index); }
double& TimeSeries::at(size_t index) { return values_.at(index); }

std::string TimeSeries::timestamp_at(size_t index) const {
    if (index < timestamps_.size()) return timestamps_[index];
    return "";
}

double TimeSeries::mean() const {
    if (values_.empty()) return 0.0;
    return std::accumulate(values_.begin(), values_.end(), 0.0) / values_.size();
}

double TimeSeries::std() const {
    if (values_.size() < 2) return 0.0;
    double m = mean();
    double sum = 0.0;
    for (double v : values_) sum += (v - m) * (v - m);
    return std::sqrt(sum / (values_.size() - 1));
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
    if (n % 2 == 0) return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
    return sorted[n / 2];
}

TimeSeries TimeSeries::normalize() const {
    double m = mean();
    double s = std();
    if (s < 1e-10) return *this;
    std::vector<double> result(values_.size());
    for (size_t i = 0; i < values_.size(); ++i) result[i] = (values_[i] - m) / s;
    return TimeSeries(result, timestamps_);
}

TimeSeries TimeSeries::min_max_scale(double min_val, double max_val) const {
    double mn = min(), mx = max();
    double range = mx - mn;
    if (range < 1e-10) return *this;
    std::vector<double> result(values_.size());
    for (size_t i = 0; i < values_.size(); ++i)
        result[i] = min_val + (values_[i] - mn) / range * (max_val - min_val);
    return TimeSeries(result, timestamps_);
}

TimeSeries TimeSeries::diff(size_t lag) const {
    if (values_.size() <= lag) return TimeSeries();
    std::vector<double> result(values_.size() - lag);
    for (size_t i = lag; i < values_.size(); ++i)
        result[i - lag] = values_[i] - values_[i - lag];
    return TimeSeries(result);
}

TimeSeries TimeSeries::log_transform() const {
    std::vector<double> result(values_.size());
    for (size_t i = 0; i < values_.size(); ++i)
        result[i] = std::log(std::max(values_[i], 1e-10));
    return TimeSeries(result, timestamps_);
}

TimeSeries TimeSeries::moving_average(size_t window_size) const {
    if (values_.size() < window_size) return *this;
    std::vector<double> result(values_.size() - window_size + 1);
    double sum = 0.0;
    for (size_t i = 0; i < window_size; ++i) sum += values_[i];
    result[0] = sum / window_size;
    for (size_t i = window_size; i < values_.size(); ++i) {
        sum += values_[i] - values_[i - window_size];
        result[i - window_size + 1] = sum / window_size;
    }
    return TimeSeries(result);
}

TimeSeries TimeSeries::exponential_smoothing(double alpha) const {
    if (values_.empty()) return *this;
    std::vector<double> result(values_.size());
    result[0] = values_[0];
    for (size_t i = 1; i < values_.size(); ++i)
        result[i] = alpha * values_[i] + (1.0 - alpha) * result[i - 1];
    return TimeSeries(result, timestamps_);
}

TimeSeries TimeSeries::resample(size_t new_size) const {
    if (values_.empty() || new_size == 0) return TimeSeries();
    std::vector<double> result(new_size);
    for (size_t i = 0; i < new_size; ++i) {
        double pos = static_cast<double>(i) / (new_size - 1) * (values_.size() - 1);
        size_t lo = static_cast<size_t>(pos);
        size_t hi = std::min(lo + 1, values_.size() - 1);
        double frac = pos - lo;
        result[i] = values_[lo] * (1.0 - frac) + values_[hi] * frac;
    }
    return TimeSeries(result);
}

std::vector<std::vector<double>> TimeSeries::create_windows(size_t window_size, size_t stride) const {
    std::vector<std::vector<double>> windows;
    for (size_t i = 0; i + window_size <= values_.size(); i += stride) {
        windows.push_back(std::vector<double>(values_.begin() + i, values_.begin() + i + window_size));
    }
    return windows;
}

std::pair<std::vector<std::vector<double>>, std::vector<double>>
TimeSeries::create_supervised_windows(size_t input_window, size_t output_window, size_t stride) const {
    std::vector<std::vector<double>> X;
    std::vector<double> y;
    for (size_t i = 0; i + input_window + output_window <= values_.size(); i += stride) {
        X.push_back(std::vector<double>(values_.begin() + i, values_.begin() + i + input_window));
        y.push_back(values_[i + input_window]);
    }
    return {X, y};
}

std::vector<double> TimeSeries::autocorrelation(size_t max_lag) const {
    std::vector<double> result(max_lag);
    double m = mean();
    double var = 0.0;
    for (double v : values_) var += (v - m) * (v - m);
    if (var < 1e-10) return result;
    for (size_t lag = 0; lag < max_lag; ++lag) {
        double sum = 0.0;
        for (size_t i = 0; i + lag < values_.size(); ++i)
            sum += (values_[i] - m) * (values_[i + lag] - m);
        result[lag] = sum / var;
    }
    return result;
}

// MultivariatTimeSeries
MultivariatTimeSeries::MultivariatTimeSeries() {}

MultivariatTimeSeries::MultivariatTimeSeries(const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& feature_names, const std::vector<std::string>& timestamps)
    : data_(data), feature_names_(feature_names), timestamps_(timestamps) {}

const std::vector<double>& MultivariatTimeSeries::feature(size_t index) const {
    return data_.at(index);
}

std::vector<double> MultivariatTimeSeries::sample(size_t index) const {
    std::vector<double> result(data_.size());
    for (size_t f = 0; f < data_.size(); ++f) result[f] = data_[f].at(index);
    return result;
}

double MultivariatTimeSeries::at(size_t feature_idx, size_t sample_idx) const {
    return data_.at(feature_idx).at(sample_idx);
}

std::vector<double> MultivariatTimeSeries::means() const {
    std::vector<double> result(data_.size());
    for (size_t f = 0; f < data_.size(); ++f) {
        if (!data_[f].empty())
            result[f] = std::accumulate(data_[f].begin(), data_[f].end(), 0.0) / data_[f].size();
    }
    return result;
}

std::vector<double> MultivariatTimeSeries::stds() const {
    auto m = means();
    std::vector<double> result(data_.size());
    for (size_t f = 0; f < data_.size(); ++f) {
        if (data_[f].size() < 2) continue;
        double sum = 0.0;
        for (double v : data_[f]) sum += (v - m[f]) * (v - m[f]);
        result[f] = std::sqrt(sum / (data_[f].size() - 1));
    }
    return result;
}

MultivariatTimeSeries MultivariatTimeSeries::normalize() const {
    auto m = means();
    auto s = stds();
    std::vector<std::vector<double>> result = data_;
    for (size_t f = 0; f < data_.size(); ++f) {
        if (s[f] < 1e-10) continue;
        for (size_t i = 0; i < data_[f].size(); ++i)
            result[f][i] = (data_[f][i] - m[f]) / s[f];
    }
    return MultivariatTimeSeries(result, feature_names_, timestamps_);
}

MultivariatTimeSeries MultivariatTimeSeries::min_max_scale() const {
    std::vector<std::vector<double>> result = data_;
    for (size_t f = 0; f < data_.size(); ++f) {
        if (data_[f].empty()) continue;
        double mn = *std::min_element(data_[f].begin(), data_[f].end());
        double mx = *std::max_element(data_[f].begin(), data_[f].end());
        double range = mx - mn;
        if (range < 1e-10) continue;
        for (size_t i = 0; i < data_[f].size(); ++i)
            result[f][i] = (data_[f][i] - mn) / range;
    }
    return MultivariatTimeSeries(result, feature_names_, timestamps_);
}

std::vector<std::vector<std::vector<double>>> MultivariatTimeSeries::create_windows(
    size_t window_size, size_t stride) const {
    std::vector<std::vector<std::vector<double>>> windows;
    size_t n_samples = num_samples();
    for (size_t i = 0; i + window_size <= n_samples; i += stride) {
        std::vector<std::vector<double>> window(data_.size());
        for (size_t f = 0; f < data_.size(); ++f)
            window[f] = std::vector<double>(data_[f].begin() + i, data_[f].begin() + i + window_size);
        windows.push_back(window);
    }
    return windows;
}

// MovingAverageForecaster
MovingAverageForecaster::MovingAverageForecaster(size_t window_size)
    : window_size_(window_size) {}

void MovingAverageForecaster::fit(const TimeSeries& ts) {
    const auto& vals = ts.values();
    size_t start = (vals.size() > window_size_) ? vals.size() - window_size_ : 0;
    last_values_ = std::vector<double>(vals.begin() + start, vals.end());
}

std::vector<double> MovingAverageForecaster::forecast(size_t steps) const {
    std::vector<double> result(steps);
    std::vector<double> buffer = last_values_;
    for (size_t i = 0; i < steps; ++i) {
        double avg = std::accumulate(buffer.begin(), buffer.end(), 0.0) / buffer.size();
        result[i] = avg;
        buffer.erase(buffer.begin());
        buffer.push_back(avg);
    }
    return result;
}

double MovingAverageForecaster::forecast_one_step() const {
    if (last_values_.empty()) return 0.0;
    return std::accumulate(last_values_.begin(), last_values_.end(), 0.0) / last_values_.size();
}

// ExponentialSmoothingForecaster
ExponentialSmoothingForecaster::ExponentialSmoothingForecaster(double alpha, double beta, double gamma)
    : alpha_(alpha), beta_(beta), gamma_(gamma), level_(0.0), trend_(0.0) {}

void ExponentialSmoothingForecaster::fit(const TimeSeries& ts) {
    const auto& vals = ts.values();
    if (vals.empty()) return;
    level_ = vals[0];
    trend_ = (vals.size() > 1) ? vals[1] - vals[0] : 0.0;
    for (size_t i = 1; i < vals.size(); ++i) {
        double prev_level = level_;
        level_ = alpha_ * vals[i] + (1.0 - alpha_) * (level_ + trend_);
        trend_ = beta_ * (level_ - prev_level) + (1.0 - beta_) * trend_;
    }
}

std::vector<double> ExponentialSmoothingForecaster::forecast(size_t steps) const {
    std::vector<double> result(steps);
    for (size_t i = 0; i < steps; ++i) {
        result[i] = level_ + (i + 1) * trend_;
    }
    return result;
}

// AutoRegressiveModel
AutoRegressiveModel::AutoRegressiveModel(size_t order)
    : order_(order) {}

void AutoRegressiveModel::fit(const TimeSeries& ts) {
    const auto& vals = ts.values();
    if (vals.size() <= order_) return;
    
    // Yule-Walker equations for AR model
    size_t n = vals.size();
    double m = std::accumulate(vals.begin(), vals.end(), 0.0) / n;
    
    // Compute autocorrelations
    std::vector<double> r(order_ + 1, 0.0);
    for (size_t lag = 0; lag <= order_; ++lag) {
        for (size_t i = lag; i < n; ++i)
            r[lag] += (vals[i] - m) * (vals[i - lag] - m);
        r[lag] /= n;
    }
    
    // Solve using Levinson-Durbin recursion
    coefficients_.resize(order_, 0.0);
    if (std::abs(r[0]) < 1e-10) return;
    
    std::vector<double> a(order_, 0.0);
    a[0] = r[1] / r[0];
    coefficients_[0] = a[0];
    
    for (size_t p = 1; p < order_; ++p) {
        double sum = r[p + 1];
        for (size_t j = 0; j < p; ++j) sum -= a[j] * r[p - j];
        double error = r[0];
        for (size_t j = 0; j < p; ++j) error -= a[j] * r[j + 1];
        if (std::abs(error) < 1e-10) break;
        
        double k = sum / error;
        std::vector<double> new_a(order_, 0.0);
        new_a[p] = k;
        for (size_t j = 0; j < p; ++j) new_a[j] = a[j] - k * a[p - 1 - j];
        a = new_a;
        for (size_t j = 0; j <= p; ++j) coefficients_[j] = a[j];
    }
    
    last_values_ = std::vector<double>(vals.end() - order_, vals.end());
}

std::vector<double> AutoRegressiveModel::forecast(size_t steps) const {
    std::vector<double> result(steps);
    std::vector<double> buffer = last_values_;
    for (size_t i = 0; i < steps; ++i) {
        double pred = 0.0;
        for (size_t j = 0; j < order_ && j < buffer.size(); ++j)
            pred += coefficients_[j] * buffer[buffer.size() - 1 - j];
        result[i] = pred;
        buffer.push_back(pred);
    }
    return result;
}

// Seasonal decomposition
SeasonalDecomposition seasonal_decompose(const TimeSeries& ts, size_t period) {
    // Simple additive decomposition
    const auto& vals = ts.values();
    size_t n = vals.size();
    
    // Compute trend with moving average
    TimeSeries trend_ts = ts.moving_average(period);
    std::vector<double> trend_vals(n, 0.0);
    size_t offset = (period - 1) / 2;
    for (size_t i = 0; i < trend_ts.size(); ++i)
        trend_vals[i + offset] = trend_ts.values()[i];
    // Pad edges
    for (size_t i = 0; i < offset; ++i) trend_vals[i] = trend_vals[offset];
    for (size_t i = offset + trend_ts.size(); i < n; ++i) trend_vals[i] = trend_vals[offset + trend_ts.size() - 1];
    
    // Compute seasonal
    std::vector<double> seasonal_vals(n, 0.0);
    std::vector<double> seasonal_avg(period, 0.0);
    std::vector<int> count(period, 0);
    for (size_t i = 0; i < n; ++i) {
        seasonal_avg[i % period] += vals[i] - trend_vals[i];
        count[i % period]++;
    }
    for (size_t i = 0; i < period; ++i) {
        if (count[i] > 0) seasonal_avg[i] /= count[i];
    }
    for (size_t i = 0; i < n; ++i) seasonal_vals[i] = seasonal_avg[i % period];
    
    // Residual
    std::vector<double> residual_vals(n);
    for (size_t i = 0; i < n; ++i) residual_vals[i] = vals[i] - trend_vals[i] - seasonal_vals[i];
    
    return {TimeSeries(trend_vals), TimeSeries(seasonal_vals), TimeSeries(residual_vals)};
}

// Utility functions
std::vector<double> detect_outliers_zscore(const TimeSeries& ts, double threshold) {
    double m = ts.mean();
    double s = ts.std();
    std::vector<double> outliers;
    if (s < 1e-10) return outliers;
    for (size_t i = 0; i < ts.size(); ++i) {
        if (std::abs(ts.values()[i] - m) / s > threshold) outliers.push_back(static_cast<double>(i));
    }
    return outliers;
}

std::vector<double> detect_outliers_iqr(const TimeSeries& ts, double multiplier) {
    std::vector<double> sorted = ts.values();
    std::sort(sorted.begin(), sorted.end());
    size_t n = sorted.size();
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;
    double lower = q1 - multiplier * iqr;
    double upper = q3 + multiplier * iqr;
    std::vector<double> outliers;
    for (size_t i = 0; i < ts.size(); ++i) {
        if (ts.values()[i] < lower || ts.values()[i] > upper) outliers.push_back(static_cast<double>(i));
    }
    return outliers;
}

TimeSeries interpolate_missing(const TimeSeries& ts, const std::vector<size_t>& missing_indices) {
    std::vector<double> result = ts.values();
    for (size_t idx : missing_indices) {
        if (idx >= result.size()) continue;
        // Linear interpolation
        size_t left = idx, right = idx;
        while (left > 0 && std::find(missing_indices.begin(), missing_indices.end(), left) != missing_indices.end()) left--;
        while (right < result.size() - 1 && std::find(missing_indices.begin(), missing_indices.end(), right) != missing_indices.end()) right++;
        if (left != right) {
            double frac = static_cast<double>(idx - left) / (right - left);
            result[idx] = result[left] * (1.0 - frac) + result[right] * frac;
        }
    }
    return TimeSeries(result);
}

} // namespace time_series
} // namespace ml
