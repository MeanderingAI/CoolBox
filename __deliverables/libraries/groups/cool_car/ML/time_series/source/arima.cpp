#include "arima.h"
#include <numeric>
#include <cmath>

namespace ml {
namespace time_series {

ARIMAModel::ARIMAModel(size_t p, size_t d, size_t q)
    : p_(p), d_(d), q_(q) {}

void ARIMAModel::fit(const TimeSeries& ts) {
    // Minimal stub: difference the series d times, then fit AR and MA parts
    // For real ARIMA, use a library or implement full estimation
    std::vector<double> vals = ts.values();
    for (size_t i = 0; i < d_; ++i) {
        std::vector<double> diffed(vals.size() - 1);
        for (size_t j = 1; j < vals.size(); ++j)
            diffed[j - 1] = vals[j] - vals[j - 1];
        vals = diffed;
    }
    mean_ = std::accumulate(vals.begin(), vals.end(), 0.0) / vals.size();
    // AR/MA estimation is omitted for brevity
    ar_coeffs_.assign(p_, 0.0);
    ma_coeffs_.assign(q_, 0.0);
    last_values_ = std::vector<double>(vals.end() - std::min<size_t>(vals.size(), p_), vals.end());
}

std::vector<double> ARIMAModel::forecast(size_t steps) const {
    std::vector<double> result(steps, mean_);
    // Minimal stub: just return the mean
    return result;
}

} // namespace time_series
} // namespace ml
