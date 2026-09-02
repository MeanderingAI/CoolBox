#include "arima.h"
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace time_series {

ARIMAModel::ARIMAModel(size_t p, size_t d, size_t q)
    : p_(p), d_(d), q_(q),
      ar_coeffs_(p, 0.0),
      ma_coeffs_(q, 0.0)
{}

void ARIMAModel::fit(const TimeSeries& ts) {
    const auto& vals = ts.values();
    if (vals.empty()) return;

    // Apply differencing d times
    std::vector<double> diff = vals;
    for (size_t i = 0; i < d_; ++i) {
        std::vector<double> tmp(diff.size() - 1);
        for (size_t j = 1; j < diff.size(); ++j)
            tmp[j - 1] = diff[j] - diff[j - 1];
        diff = tmp;
    }

    if (diff.empty()) return;

    size_t n = diff.size();
    mean_ = std::accumulate(diff.begin(), diff.end(), 0.0) / n;

    // Store last p values (undifferenced) for forecasting
    last_values_.assign(vals.end() - std::min(p_ + d_, vals.size()),
                         vals.end());

    std::vector<double> centered(n);
    for (size_t i = 0; i < n; ++i)
        centered[i] = diff[i] - mean_;

    // Compute autocorrelations
    double var = 0.0;
    for (auto v : centered) var += v * v;
    if (var == 0.0) return;

    std::vector<double> acf(p_ + 1, 0.0);
    for (size_t lag = 0; lag <= p_; ++lag) {
        double s = 0.0;
        for (size_t i = lag; i < n; ++i)
            s += centered[i] * centered[i - lag];
        acf[lag] = s / var;
    }

    for (size_t i = 0; i < p_ && i < acf.size() - 1; ++i)
        ar_coeffs_[i] = acf[i + 1] / (acf[0] + 1e-10);

    // Compute residuals
    residuals_.assign(n, 0.0);
    for (size_t t = p_; t < n; ++t) {
        double pred = mean_;
        for (size_t i = 0; i < p_; ++i)
            pred += ar_coeffs_[i] * centered[t - 1 - i];
        residuals_[t] = centered[t] - (pred - mean_);
    }

    for (size_t i = 0; i < q_; ++i) {
        double num = 0.0, den = 0.0;
        for (size_t t = p_ + i + 1; t < n; ++t) {
            num += residuals_[t] * residuals_[t - i - 1];
            den += residuals_[t - i - 1] * residuals_[t - i - 1];
        }
        ma_coeffs_[i] = (den > 1e-10) ? num / den : 0.0;
    }
}

std::vector<double> ARIMAModel::forecast(size_t steps) const {
    std::vector<double> result;
    result.reserve(steps);

    std::vector<double> history = last_values_;
    std::vector<double> errors(q_, 0.0);

    for (size_t s = 0; s < steps; ++s) {
        double pred = mean_;
        // Use differenced history for AR part
        if (history.size() >= 2 && d_ > 0) {
            double last_diff = history.back() - history[history.size() - 2];
            pred += ar_coeffs_.empty() ? 0.0 : ar_coeffs_[0] * last_diff;
        }
        for (size_t i = 0; i < q_; ++i)
            pred += ma_coeffs_[i] * errors[i];

        // Integrate (inverse differencing): add to last value
        double next = (d_ > 0 && !history.empty()) ? history.back() + pred : pred;
        result.push_back(next);
        history.push_back(next);

        for (size_t i = q_ - 1; i > 0; --i)
            errors[i] = errors[i - 1];
        if (q_ > 0) errors[0] = 0.0;
    }
    return result;
}

} // namespace time_series
} // namespace ml
