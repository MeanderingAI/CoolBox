#include <Rcpp.h>
#include "linear_regression.h"

#include "../../../packages/ML/generalized_linear_model/source/generalized_linear_model.cpp"
#include "../../../packages/ML/generalized_linear_model/source/linear_regression.cpp"

namespace {
std::vector<std::vector<double>> to_std_matrix(const Rcpp::NumericMatrix& x) {
    std::vector<std::vector<double>> out(x.nrow(), std::vector<double>(x.ncol()));
    for (int i = 0; i < x.nrow(); ++i) {
        for (int j = 0; j < x.ncol(); ++j) {
            out[i][j] = x(i, j);
        }
    }
    return out;
}
}

extern "C" SEXP _coolboxr_fit_linear_regression(
    SEXP xSEXP,
    SEXP ySEXP,
    SEXP methodSEXP,
    SEXP iterationsSEXP,
    SEXP learningRateSEXP) {
    BEGIN_RCPP

    Rcpp::NumericMatrix x(xSEXP);
    Rcpp::NumericVector y(ySEXP);
    std::string method = Rcpp::as<std::string>(methodSEXP);
    int iterations = Rcpp::as<int>(iterationsSEXP);
    double learning_rate = Rcpp::as<double>(learningRateSEXP);

    if (x.nrow() != y.size()) {
        Rcpp::stop("`x` and `y` must have compatible dimensions.");
    }

    LinearRegressionFitMethod::Type fit_type =
        method == "gradient_descent"
            ? LinearRegressionFitMethod::GRADIENT_DESCENT
            : LinearRegressionFitMethod::CLOSED_FORM;

    LinearRegressionFitMethod fit_method(
        static_cast<unsigned int>(iterations),
        learning_rate,
        fit_type
    );

    LinearRegression model(fit_method);
    model.fit(to_std_matrix(x), Rcpp::as<std::vector<double>>(y));

    auto coefficients = model.get_coefficients();

    return Rcpp::List::create(
        Rcpp::Named("weights") = coefficients.first,
        Rcpp::Named("intercept") = coefficients.second,
        Rcpp::Named("method") = method,
        Rcpp::Named("iterations") = iterations,
        Rcpp::Named("learning_rate") = learning_rate,
        Rcpp::Named("feature_count") = x.ncol()
    );

    END_RCPP
}

extern "C" SEXP _coolboxr_predict_linear_regression(SEXP modelSEXP, SEXP newdataSEXP) {
    BEGIN_RCPP

    Rcpp::List model(modelSEXP);
    Rcpp::NumericVector weights = model["weights"];
    double intercept = Rcpp::as<double>(model["intercept"]);
    Rcpp::NumericMatrix newdata(newdataSEXP);

    if (newdata.ncol() != weights.size()) {
        Rcpp::stop("`newdata` column count must match the fitted weights.");
    }

    Rcpp::NumericVector predictions(newdata.nrow());
    for (int i = 0; i < newdata.nrow(); ++i) {
        double fitted = intercept;
        for (int j = 0; j < newdata.ncol(); ++j) {
            fitted += newdata(i, j) * weights[j];
        }
        predictions[i] = fitted;
    }

    return predictions;

    END_RCPP
}
