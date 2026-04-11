#ifndef COOLBOX_COOLBOX_C_H
#define COOLBOX_COOLBOX_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxClient CoolBoxClient;

/**
 * Returns the default endpoint used by first-party bindings.
 */
const char *coolbox_c_default_endpoint(void);

/**
 * Creates a client using the default endpoint.
 */
CoolBoxClient *coolbox_c_create_default_client(void);

/**
 * Creates a client using the provided endpoint string.
 */
CoolBoxClient *coolbox_c_create_client(const char *endpoint);

/**
 * Destroys a client created by this API.
 */
void coolbox_c_destroy_client(CoolBoxClient *client);

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

/**
 * Returns the client endpoint string.
 */
const char *coolbox_c_client_endpoint(const CoolBoxClient *client);

/**
 * Returns the scaffold version string for the client.
 */
const char *coolbox_c_client_version(const CoolBoxClient *client);

/**
 * Returns a short description of the client bindings package.
 */
const char *coolbox_c_client_describe(const CoolBoxClient *client);

/**
 * Returns the number of advertised client capabilities.
 */
size_t coolbox_c_client_capability_count(const CoolBoxClient *client);

/**
 * Returns the client capability name at the given zero-based index.
 * Returns NULL when the index is out of range.
 */
const char *coolbox_c_client_capability_at(const CoolBoxClient *client, size_t index);

/**
 * Returns non-zero when the client is ready to use.
 */
int coolbox_c_client_is_ready(const CoolBoxClient *client);

#ifdef __cplusplus
}
#endif

#endif
