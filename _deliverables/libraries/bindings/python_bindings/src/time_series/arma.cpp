#include "arma.h"
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace time_series {

ARMAModel::ARMAModel(size_t p, size_t q)
    : p_(p), q_(q),
      ar_coeffs_(p, 0.0),
      ma_coeffs_(q, 0.0)
{}

void ARMAModel::fit(const TimeSeries& ts) {
    const auto& vals = ts.values();
    if (vals.empty()) return;

    // Compute mean
    mean_ = std::accumulate(vals.begin(), vals.end(), 0.0) / vals.size();

    // Simple Yule-Walker estimate for AR coefficients (approximate)
    size_t n = vals.size();
    last_values_.assign(vals.end() - std::min(p_, n), vals.end());

    // Initialize AR coefficients via autocorrelation (simplified)
    std::vector<double> centered(n);
    for (size_t i = 0; i < n; ++i)
        centered[i] = vals[i] - mean_;

    // Compute autocorrelations up to lag p
    std::vector<double> acf(p_ + 1, 0.0);
    double var = 0.0;
    for (size_t i = 0; i < n; ++i)
        var += centered[i] * centered[i];
    if (var == 0.0) return;

    for (size_t lag = 0; lag <= p_; ++lag) {
        double s = 0.0;
        for (size_t i = lag; i < n; ++i)
            s += centered[i] * centered[i - lag];
        acf[lag] = s / var;
    }

    // Levinson-Durbin (order 1 approximation for simplicity)
    for (size_t i = 0; i < p_ && i < acf.size() - 1; ++i)
        ar_coeffs_[i] = acf[i + 1] / (acf[0] + 1e-10);

    // Compute residuals for MA part
    residuals_.assign(n, 0.0);
    for (size_t t = p_; t < n; ++t) {
        double pred = mean_;
        for (size_t i = 0; i < p_; ++i)
            pred += ar_coeffs_[i] * (centered[t - 1 - i]);
        residuals_[t] = centered[t] - (pred - mean_);
    }

    // Simple MA coefficient estimate
    for (size_t i = 0; i < q_; ++i) {
        double num = 0.0, den = 0.0;
        for (size_t t = p_ + i + 1; t < n; ++t) {
            num += residuals_[t] * residuals_[t - i - 1];
            den += residuals_[t - i - 1] * residuals_[t - i - 1];
        }
        ma_coeffs_[i] = (den > 1e-10) ? num / den : 0.0;
    }
}

std::vector<double> ARMAModel::forecast(size_t steps) const {
    std::vector<double> result;
    result.reserve(steps);

    std::vector<double> history = last_values_;
    std::vector<double> errors(q_, 0.0);

    for (size_t s = 0; s < steps; ++s) {
        double pred = mean_;
        for (size_t i = 0; i < p_ && i < history.size(); ++i)
            pred += ar_coeffs_[i] * (history[history.size() - 1 - i] - mean_);
        for (size_t i = 0; i < q_; ++i)
            pred += ma_coeffs_[i] * errors[i];

        result.push_back(pred);
        history.push_back(pred);
        // Shift errors: new error is 0 for forecast
        for (size_t i = q_ - 1; i > 0; --i)
            errors[i] = errors[i - 1];
        if (q_ > 0) errors[0] = 0.0;
    }
    return result;
}

} // namespace time_series
} // namespace ml
