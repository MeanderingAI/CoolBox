#include "arma.h"
#include <numeric>
#include <cmath>

namespace ml {
namespace time_series {

ARMAModel::ARMAModel(size_t p, size_t q)
    : p_(p), q_(q) {}

void ARMAModel::fit(const TimeSeries& ts) {
    // Minimal stub: fit AR and MA parts
    const auto& vals = ts.values();
    mean_ = std::accumulate(vals.begin(), vals.end(), 0.0) / vals.size();
    // AR/MA estimation is omitted for brevity
    ar_coeffs_.assign(p_, 0.0);
    ma_coeffs_.assign(q_, 0.0);
    last_values_ = std::vector<double>(vals.end() - std::min<size_t>(vals.size(), p_), vals.end());
}

std::vector<double> ARMAModel::forecast(size_t steps) const {
    std::vector<double> result(steps, mean_);
    // Minimal stub: just return the mean
    return result;
}

} // namespace time_series
} // namespace ml
