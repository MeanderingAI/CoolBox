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
typedef struct CoolBoxSqlDatabase CoolBoxSqlDatabase;
typedef struct CoolBoxSqlSchema CoolBoxSqlSchema;
typedef struct CoolBoxSqlRepository CoolBoxSqlRepository;
typedef struct CoolBoxRouteServer CoolBoxRouteServer;

typedef char *(*CoolBoxRouteHandler)(
    void *context, const char *method, const char *path, const char *body,
    const char *const *path_param_keys, const char *const *path_param_values, size_t path_param_count,
    const char *const *query_param_keys, const char *const *query_param_values, size_t query_param_count,
    int *status_code);
/* Route handlers must return a string allocated by coolbox_c_duplicate_string. */

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

COOLBOX_C_API void coolbox_c_free_string(char *value);
COOLBOX_C_API char *coolbox_c_duplicate_string(const char *value);

COOLBOX_C_API CoolBoxSqlDatabase *coolbox_sql_database_create(
    const char *provider, char *error, size_t error_capacity);
COOLBOX_C_API void coolbox_sql_database_destroy(CoolBoxSqlDatabase *database);
COOLBOX_C_API int coolbox_sql_database_connect(
    CoolBoxSqlDatabase *database, const char *connection_string,
    char *error, size_t error_capacity);
COOLBOX_C_API void coolbox_sql_database_disconnect(CoolBoxSqlDatabase *database);
COOLBOX_C_API char *coolbox_sql_database_execute(
    CoolBoxSqlDatabase *database, const char *query,
    const char *const *params, size_t param_count,
    char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_sql_database_begin(CoolBoxSqlDatabase *database);
COOLBOX_C_API int coolbox_sql_database_commit(CoolBoxSqlDatabase *database);
COOLBOX_C_API int coolbox_sql_database_rollback(CoolBoxSqlDatabase *database);

COOLBOX_C_API CoolBoxSqlSchema *coolbox_sql_schema_parse(
    const char *schema_text, char *error, size_t error_capacity);
COOLBOX_C_API void coolbox_sql_schema_destroy(CoolBoxSqlSchema *schema);
COOLBOX_C_API size_t coolbox_sql_schema_model_count(const CoolBoxSqlSchema *schema);
COOLBOX_C_API char *coolbox_sql_schema_model_name(const CoolBoxSqlSchema *schema, size_t index);
COOLBOX_C_API int coolbox_sql_migrate_up(
    CoolBoxSqlDatabase *database, const CoolBoxSqlSchema *schema,
    char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_sql_migrations_up(
    CoolBoxSqlDatabase *database, const char *directory, char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_sql_migrations_down(
    CoolBoxSqlDatabase *database, const char *directory, size_t steps,
    char *error, size_t error_capacity);
COOLBOX_C_API char *coolbox_sql_migrations_current_version(
    CoolBoxSqlDatabase *database, const char *directory, char *error, size_t error_capacity);
COOLBOX_C_API CoolBoxSqlRepository *coolbox_sql_repository_create(
    CoolBoxSqlDatabase *database, const CoolBoxSqlSchema *schema,
    const char *model_name, char *error, size_t error_capacity);
COOLBOX_C_API void coolbox_sql_repository_destroy(CoolBoxSqlRepository *repository);
COOLBOX_C_API char *coolbox_sql_repository_call(
    CoolBoxSqlRepository *repository, const char *operation, const char *id,
    const char *const *keys, const char *const *values, size_t count,
    const char *const *condition_keys, const char *const *condition_values,
    size_t condition_count, char *error, size_t error_capacity);

COOLBOX_C_API CoolBoxRouteServer *coolbox_http_server_create(
    int port, char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_http_server_add_route(
    CoolBoxRouteServer *server, const char *method, const char *path,
    CoolBoxRouteHandler handler, void *context, char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_http_server_register_handler(
    CoolBoxRouteServer *server, const char *name,
    CoolBoxRouteHandler handler, void *context, char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_http_server_load_routes(
    CoolBoxRouteServer *server, const char *routes_file, char *error, size_t error_capacity);
COOLBOX_C_API int coolbox_http_server_start(
    CoolBoxRouteServer *server, char *error, size_t error_capacity);
COOLBOX_C_API void coolbox_http_server_stop(CoolBoxRouteServer *server);
COOLBOX_C_API int coolbox_http_server_is_running(const CoolBoxRouteServer *server);
COOLBOX_C_API void coolbox_http_server_destroy(CoolBoxRouteServer *server);

#ifdef __cplusplus
}
#endif

#endif
