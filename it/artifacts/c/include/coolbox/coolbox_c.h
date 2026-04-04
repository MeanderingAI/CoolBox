#ifndef COOLBOX_COOLBOX_C_H
#define COOLBOX_COOLBOX_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the scaffold version string for the CoolBox C bindings.
 */
const char *coolbox_c_version(void);

/**
 * Returns a short description of the C bindings package.
 */
const char *coolbox_c_describe(void);

/**
 * Returns the number of advertised capabilities.
 */
size_t coolbox_c_capability_count(void);

/**
 * Returns the capability name at the given zero-based index.
 * Returns NULL when the index is out of range.
 */
const char *coolbox_c_capability_at(size_t index);

/**
 * Returns non-zero when the bindings are ready to use.
 */
int coolbox_c_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif
