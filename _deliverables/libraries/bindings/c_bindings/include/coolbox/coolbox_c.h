#ifndef COOLBOX_COOLBOX_C_H
#define COOLBOX_COOLBOX_C_H

#include <stddef.h>

#if defined(_WIN32) && defined(COOLBOX_C_BINDINGS_SHARED)
#if defined(COOLBOX_C_BINDINGS_EXPORTS)
#define COOLBOX_C_API __declspec(dllexport)
#else
#define COOLBOX_C_API __declspec(dllimport)
#endif
#else
#define COOLBOX_C_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxClient CoolBoxClient;

/**
 * Returns the default endpoint used by first-party bindings.
 */
COOLBOX_C_API const char *coolbox_c_default_endpoint(void);

/**
 * Creates a client using the default endpoint.
 */
COOLBOX_C_API CoolBoxClient *coolbox_c_create_default_client(void);

/**
 * Creates a client using the provided endpoint string.
 */
COOLBOX_C_API CoolBoxClient *coolbox_c_create_client(const char *endpoint);

/**
 * Destroys a client created by this API.
 */
COOLBOX_C_API void coolbox_c_destroy_client(CoolBoxClient *client);

/**
 * Returns the scaffold version string for the CoolBox C bindings.
 */
COOLBOX_C_API const char *coolbox_c_version(void);

/**
 * Returns a short description of the C bindings package.
 */
COOLBOX_C_API const char *coolbox_c_describe(void);

/**
 * Returns the number of advertised capabilities.
 */
COOLBOX_C_API size_t coolbox_c_capability_count(void);

/**
 * Returns the capability name at the given zero-based index.
 * Returns NULL when the index is out of range.
 */
COOLBOX_C_API const char *coolbox_c_capability_at(size_t index);

/**
 * Returns non-zero when the bindings are ready to use.
 */
COOLBOX_C_API int coolbox_c_is_ready(void);

/**
 * Returns the client endpoint string.
 */
COOLBOX_C_API const char *coolbox_c_client_endpoint(const CoolBoxClient *client);

/**
 * Returns the scaffold version string for the client.
 */
COOLBOX_C_API const char *coolbox_c_client_version(const CoolBoxClient *client);

/**
 * Returns a short description of the client bindings package.
 */
COOLBOX_C_API const char *coolbox_c_client_describe(const CoolBoxClient *client);

/**
 * Returns the number of advertised client capabilities.
 */
COOLBOX_C_API size_t coolbox_c_client_capability_count(const CoolBoxClient *client);

/**
 * Returns the client capability name at the given zero-based index.
 * Returns NULL when the index is out of range.
 */
COOLBOX_C_API const char *coolbox_c_client_capability_at(const CoolBoxClient *client, size_t index);

/**
 * Returns non-zero when the client is ready to use.
 */
COOLBOX_C_API int coolbox_c_client_is_ready(const CoolBoxClient *client);

/**
 * UUID generation APIs backed by trekker/MISC/uuid_generation.
 * Returned string pointers are valid until the next UUID call on the same thread.
 */
COOLBOX_C_API const char *coolbox_c_uuid_v1(void);
COOLBOX_C_API const char *coolbox_c_uuid_v2(unsigned int local_identifier, unsigned int local_domain);
COOLBOX_C_API const char *coolbox_c_uuid_v3(const char *namespace_uuid, const char *name);
COOLBOX_C_API const char *coolbox_c_uuid_v4(void);
COOLBOX_C_API const char *coolbox_c_uuid_v5(const char *namespace_uuid, const char *name);
COOLBOX_C_API const char *coolbox_c_uuid_v6(void);
COOLBOX_C_API const char *coolbox_c_uuid_v8(const char *custom_entropy_hex);
COOLBOX_C_API const char *coolbox_c_guid(void);
COOLBOX_C_API const char *coolbox_c_cuid(void);

#ifdef __cplusplus
}
#endif

#endif
