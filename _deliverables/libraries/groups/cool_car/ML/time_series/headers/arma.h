#ifndef ARMA_H
#define ARMA_H

#include "time_series.h"
#include <vector>

namespace ml {
namespace time_series {

class ARMAModel {
public:
    ARMAModel(size_t p, size_t q);
    void fit(const TimeSeries& ts);
    std::vector<double> forecast(size_t steps) const;
    // Optionally: accessors for parameters, residuals, etc.
private:
    size_t p_, q_;
    std::vector<double> ar_coeffs_;
    std::vector<double> ma_coeffs_;
    double mean_ = 0.0;
    std::vector<double> residuals_;
    std::vector<double> last_values_;
};

} // namespace time_series
} // namespace ml

#endif // ARMA_H
