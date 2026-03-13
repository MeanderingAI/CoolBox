#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern SEXP _coolboxr_fit_linear_regression(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_predict_linear_regression(SEXP, SEXP);

static const R_CallMethodDef CallEntries[] = {
    {"_coolboxr_fit_linear_regression", (DL_FUNC) &_coolboxr_fit_linear_regression, 5},
    {"_coolboxr_predict_linear_regression", (DL_FUNC) &_coolboxr_predict_linear_regression, 2},
    {NULL, NULL, 0}
};

void R_init_coolboxr(DllInfo* dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
