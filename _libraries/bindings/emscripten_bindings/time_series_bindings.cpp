#include <emscripten/bind.h>
#include "time_series.h"

using namespace emscripten;
using namespace ml::time_series;

EMSCRIPTEN_BINDINGS(time_series_module) {
    register_vector<double>("VectorDouble_TS");
    register_vector<std::string>("VectorString_TS");

    class_<TimeSeries>("TimeSeries")
        .constructor<>()
        .constructor<const std::vector<double>&, const std::vector<std::string>&>()
        .function("mean", &TimeSeries::mean)
        .function("std", &TimeSeries::std)
        .function("min", &TimeSeries::min)
        .function("max", &TimeSeries::max)
        .function("median", &TimeSeries::median)
    ;

    class_<MovingAverageForecaster>("MovingAverageForecaster")
        .constructor<size_t>()
        .function("fit", &MovingAverageForecaster::fit)
        .function("forecast_one_step", &MovingAverageForecaster::forecast_one_step)
    ;

    class_<ExponentialSmoothingForecaster>("ExponentialSmoothingForecaster")
        .constructor<double, double, double>()
        .function("fit", &ExponentialSmoothingForecaster::fit)
    ;

    class_<AutoRegressiveModel>("AutoRegressiveModel")
        .constructor<size_t>()
        .function("fit", &AutoRegressiveModel::fit)
    ;
}
