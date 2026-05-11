#include <Rcpp.h>
#include "time_series.h"

#include "../../../packages/ML/time_series/source/time_series.cpp"

namespace {
ml::time_series::TimeSeries to_time_series(const Rcpp::NumericVector& values) {
    return ml::time_series::TimeSeries(Rcpp::as<std::vector<double>>(values));
}

Rcpp::NumericVector to_numeric_vector(const std::vector<double>& values) {
    return Rcpp::wrap(values);
}
}

extern "C" SEXP _coolboxr_time_series_summary(SEXP valuesSEXP, SEXP maxLagSEXP) {
    BEGIN_RCPP

    Rcpp::NumericVector values(valuesSEXP);
    int max_lag = Rcpp::as<int>(maxLagSEXP);
    ml::time_series::TimeSeries ts = to_time_series(values);

    return Rcpp::List::create(
        Rcpp::Named("length") = values.size(),
        Rcpp::Named("mean") = ts.mean(),
        Rcpp::Named("std") = ts.std(),
        Rcpp::Named("min") = ts.min(),
        Rcpp::Named("median") = ts.median(),
        Rcpp::Named("max") = ts.max(),
        Rcpp::Named("autocorrelation") = to_numeric_vector(ts.autocorrelation(static_cast<size_t>(max_lag)))
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_transform_time_series(
    SEXP valuesSEXP,
    SEXP methodSEXP,
    SEXP primarySEXP,
    SEXP secondarySEXP,
    SEXP tertiarySEXP) {
    BEGIN_RCPP

    Rcpp::NumericVector values(valuesSEXP);
    std::string method = Rcpp::as<std::string>(methodSEXP);
    double primary = Rcpp::as<double>(primarySEXP);
    double secondary = Rcpp::as<double>(secondarySEXP);
    (void) tertiarySEXP;

    ml::time_series::TimeSeries ts = to_time_series(values);
    ml::time_series::TimeSeries transformed;

    if (method == "normalize") {
        transformed = ts.normalize();
    } else if (method == "min_max_scale") {
        transformed = ts.min_max_scale(primary, secondary);
    } else if (method == "diff") {
        transformed = ts.diff(static_cast<size_t>(primary));
    } else if (method == "log_transform") {
        transformed = ts.log_transform();
    } else if (method == "moving_average") {
        transformed = ts.moving_average(static_cast<size_t>(primary));
    } else if (method == "exponential_smoothing") {
        transformed = ts.exponential_smoothing(primary);
    } else if (method == "resample") {
        transformed = ts.resample(static_cast<size_t>(primary));
    } else {
        Rcpp::stop("Unknown time-series transform method.");
    }

    return to_numeric_vector(transformed.values());

    END_RCPP
}

extern "C" SEXP _coolboxr_forecast_time_series(
    SEXP valuesSEXP,
    SEXP methodSEXP,
    SEXP stepsSEXP,
    SEXP primarySEXP,
    SEXP secondarySEXP,
    SEXP tertiarySEXP) {
    BEGIN_RCPP

    Rcpp::NumericVector values(valuesSEXP);
    std::string method = Rcpp::as<std::string>(methodSEXP);
    int steps = Rcpp::as<int>(stepsSEXP);
    double primary = Rcpp::as<double>(primarySEXP);
    double secondary = Rcpp::as<double>(secondarySEXP);
    double tertiary = Rcpp::as<double>(tertiarySEXP);

    ml::time_series::TimeSeries ts = to_time_series(values);
    std::vector<double> forecast;

    if (method == "moving_average") {
        ml::time_series::MovingAverageForecaster model(static_cast<size_t>(primary));
        model.fit(ts);
        forecast = model.forecast(static_cast<size_t>(steps));
    } else if (method == "exponential_smoothing") {
        ml::time_series::ExponentialSmoothingForecaster model(primary, secondary, tertiary);
        model.fit(ts);
        forecast = model.forecast(static_cast<size_t>(steps));
    } else if (method == "autoregressive") {
        ml::time_series::AutoRegressiveModel model(static_cast<size_t>(primary));
        model.fit(ts);
        forecast = model.forecast(static_cast<size_t>(steps));
    } else {
        Rcpp::stop("Unknown time-series forecasting method.");
    }

    return to_numeric_vector(forecast);

    END_RCPP
}

extern "C" SEXP _coolboxr_detect_time_series_outliers(
    SEXP valuesSEXP,
    SEXP methodSEXP,
    SEXP parameterSEXP) {
    BEGIN_RCPP

    Rcpp::NumericVector values(valuesSEXP);
    std::string method = Rcpp::as<std::string>(methodSEXP);
    double parameter = Rcpp::as<double>(parameterSEXP);

    ml::time_series::TimeSeries ts = to_time_series(values);
    std::vector<double> outliers;

    if (method == "zscore") {
        outliers = ml::time_series::detect_outliers_zscore(ts, parameter);
    } else if (method == "iqr") {
        outliers = ml::time_series::detect_outliers_iqr(ts, parameter);
    } else {
        Rcpp::stop("Unknown time-series outlier detection method.");
    }

    Rcpp::IntegerVector indices(outliers.size());
    for (R_xlen_t i = 0; i < indices.size(); ++i) {
        indices[i] = static_cast<int>(outliers[static_cast<size_t>(i)]) + 1;
    }

    return indices;

    END_RCPP
}