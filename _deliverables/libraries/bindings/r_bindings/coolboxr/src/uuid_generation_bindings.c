#include <R.h>
#include <Rinternals.h>

#include "coolbox/coolbox_c.h"

SEXP _coolboxr_uuid_v1(void) {
    return Rf_mkString(coolbox_c_uuid_v1());
}

SEXP _coolboxr_uuid_v2(SEXP localIdentifierSEXP, SEXP localDomainSEXP) {
    unsigned int local_identifier = (unsigned int) Rf_asInteger(localIdentifierSEXP);
    unsigned int local_domain = (unsigned int) Rf_asInteger(localDomainSEXP);
    return Rf_mkString(coolbox_c_uuid_v2(local_identifier, local_domain));
}

SEXP _coolboxr_uuid_v3(SEXP namespaceSEXP, SEXP nameSEXP) {
    const char* ns = Rf_translateCharUTF8(STRING_ELT(namespaceSEXP, 0));
    const char* name = Rf_translateCharUTF8(STRING_ELT(nameSEXP, 0));
    return Rf_mkString(coolbox_c_uuid_v3(ns, name));
}

SEXP _coolboxr_uuid_v4(void) {
    return Rf_mkString(coolbox_c_uuid_v4());
}

SEXP _coolboxr_uuid_v5(SEXP namespaceSEXP, SEXP nameSEXP) {
    const char* ns = Rf_translateCharUTF8(STRING_ELT(namespaceSEXP, 0));
    const char* name = Rf_translateCharUTF8(STRING_ELT(nameSEXP, 0));
    return Rf_mkString(coolbox_c_uuid_v5(ns, name));
}

SEXP _coolboxr_uuid_v6(void) {
    return Rf_mkString(coolbox_c_uuid_v6());
}

SEXP _coolboxr_uuid_v8(SEXP entropyHexSEXP) {
    const char* entropy = Rf_translateCharUTF8(STRING_ELT(entropyHexSEXP, 0));
    return Rf_mkString(coolbox_c_uuid_v8(entropy));
}

SEXP _coolboxr_guid(void) {
    return Rf_mkString(coolbox_c_guid());
}
