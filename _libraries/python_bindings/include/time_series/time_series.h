#ifndef TIME_SERIES_H
#define TIME_SERIES_H

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace ml {
namespace time_series {

// Time series data structure
class TimeSeries {
public:
    TimeSeries();
    TimeSeries(const std::vector<double>& values, 
               const std::vector<std::string>& timestamps = {});
    
    // Data access
    size_t size() const { return values_.size(); }
    const std::vector<double>& values() const { return values_; }
    std::vector<double>& values() { return values_; }
    
    double at(size_t index) const;
    double& at(size_t index);
    
    std::string timestamp_at(size_t index) const;
    
    // Statistics
    double mean() const;
    double std() const;
    double min() const;
    double max() const;
    double median() const;
    
    // Transformations
    TimeSeries normalize() const;  // Z-score normalization
    TimeSeries min_max_scale(double min_val = 0.0, double max_val = 1.0) const;
    TimeSeries diff(size_t lag = 1) const;  // Differencing
    TimeSeries log_transform() const;
    TimeSeries moving_average(size_t window_size) const;
    TimeSeries exponential_smoothing(double alpha) const;
    
    // Resampling
    TimeSeries resample(size_t new_size) const;
    
    // Windowing for ML
    std::vector<std::vector<double>> create_windows(size_t window_size, size_t stride = 1) const;
    std::pair<std::vector<std::vector<double>>, std::vector<double>> 
        create_supervised_windows(size_t input_window, size_t output_window = 1, size_t stride = 1) const;
    
    // Autocorrelation
    std::vector<double> autocorrelation(size_t max_lag) const;
    
private:
    std::vector<double> values_;
    std::vector<std::string> timestamps_;
};

// Multivariate time series
class MultivariatTimeSeries {
public:
    MultivariatTimeSeries();
    MultivariatTimeSeries(const std::vector<std::vector<double>>& data,
                          const std::vector<std::string>& feature_names = {},
                          const std::vector<std::string>& timestamps = {});
    
    // Dimensions
    size_t num_samples() const { return data_.empty() ? 0 : data_[0].size(); }
    size_t num_features() const { return data_.size(); }
    
    // Access
    const std::vector<std::vector<double>>& data() const { return data_; }
    const std::vector<double>& feature(size_t index) const;
    std::vector<double> sample(size_t index) const;
    
    double at(size_t feature_idx, size_t sample_idx) const;
    
    // Statistics per feature
    std::vector<double> means() const;
    std::vector<double> stds() const;
    
    // Transformations
    MultivariatTimeSeries normalize() const;
    MultivariatTimeSeries min_max_scale() const;
    
    // Windowing
    std::vector<std::vector<std::vector<double>>> create_windows(size_t window_size, size_t stride = 1) const;
    
private:
    std::vector<std::vector<double>> data_;  // [features][samples]
    std::vector<std::string> feature_names_;
    std::vector<std::string> timestamps_;
};

// Simple forecasting models

// Moving Average Forecast
class MovingAverageForecaster {
public:
    MovingAverageForecaster(size_t window_size);
    
    void fit(const TimeSeries& ts);
    std::vector<double> forecast(size_t steps) const;
    double forecast_one_step() const;
    
private:
    size_t window_size_;
    std::vector<double> last_values_;
};

// Exponential Smoothing
class ExponentialSmoothingForecaster {
public:
    ExponentialSmoothingForecaster(double alpha, double beta = 0.0, double gamma = 0.0);
    
    void fit(const TimeSeries& ts);
    std::vector<double> forecast(size_t steps) const;
    
private:
    double alpha_;  // Level smoothing
    double beta_;   // Trend smoothing
    double gamma_;  // Seasonality smoothing
    
    double level_;
    double trend_;
    std::vector<double> seasonal_;
};

// Auto-regressive model (AR)
class AutoRegressiveModel {
public:
    AutoRegressiveModel(size_t order);
    
    void fit(const TimeSeries& ts);
    std::vector<double> forecast(size_t steps) const;
    
    const std::vector<double>& coefficients() const { return coefficients_; }
    
private:
    size_t order_;
    std::vector<double> coefficients_;
    std::vector<double> last_values_;
};

// Seasonal decomposition
struct SeasonalDecomposition {
    TimeSeries trend;
    TimeSeries seasonal;
    TimeSeries residual;
};

SeasonalDecomposition seasonal_decompose(const TimeSeries& ts, size_t period);

// Utility functions
std::vector<double> detect_outliers_zscore(const TimeSeries& ts, double threshold = 3.0);
std::vector<double> detect_outliers_iqr(const TimeSeries& ts, double multiplier = 1.5);

TimeSeries interpolate_missing(const TimeSeries& ts, const std::vector<size_t>& missing_indices);

} // namespace time_series
} // namespace ml

#endif // TIME_SERIES_H
