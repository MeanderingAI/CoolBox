#include <R.h>
#include <Rinternals.h>

#include "coolbox/coolbox_c.h"

SEXP _coolboxr_metadata_version(void) {
    return Rf_mkString(coolbox_c_version());
}

SEXP _coolboxr_metadata_describe(void) {
    return Rf_mkString(coolbox_c_describe());
}

SEXP _coolboxr_metadata_capability_count(void) {
    return Rf_ScalarInteger((int)coolbox_c_capability_count());
}

SEXP _coolboxr_metadata_capability_at(SEXP indexSEXP) {
    int index = Rf_asInteger(indexSEXP);
    const char *value;

    if (index < 0) {
        return Rf_mkString("");
    }

    value = coolbox_c_capability_at((size_t)index);
    return Rf_mkString(value == NULL ? "" : value);
}

SEXP _coolboxr_metadata_is_ready(void) {
    return Rf_ScalarLogical(coolbox_c_is_ready() != 0);
}