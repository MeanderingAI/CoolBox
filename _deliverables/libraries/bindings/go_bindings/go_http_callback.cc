#include "coolbox/coolbox_c.h"

extern "C" char *coolbox_go_http_route_dispatch(
    void *context, const char *method, const char *path, const char *body,
    const char *const *path_keys, const char *const *path_values, size_t path_count,
    const char *const *query_keys, const char *const *query_values, size_t query_count,
    int *status_code);

extern "C" CoolBoxRouteHandler coolbox_go_http_handler_pointer(void) {
    return &coolbox_go_http_route_dispatch;
}
