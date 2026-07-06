#include "coolbox/coolbox_c.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct CoolBoxClient {
    char *endpoint;
};

static const char *const COOLBOX_C_CAPABILITIES[] = {
    "metadata",
    "version",
    "metadata_management",
    "uuid_generation"
};

static const char *const COOLBOX_C_DEFAULT_ENDPOINT = "local://coolbox";

extern const char *get_metadata_management_library_name(void);
extern const char *get_metadata_management_library_version(void);
extern const char *get_metadata_management_library_description(void);
extern const char *get_metadata_management_library_author(void);

static char *coolbox_c_dup_string(const char *value) {
    size_t length;
    char *copy;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value) + 1U;
    copy = (char *)malloc(length);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length);
    return copy;
}

const char *coolbox_c_default_endpoint(void) {
    return COOLBOX_C_DEFAULT_ENDPOINT;
}

CoolBoxClient *coolbox_c_create_default_client(void) {
    return coolbox_c_create_client(COOLBOX_C_DEFAULT_ENDPOINT);
}

CoolBoxClient *coolbox_c_create_client(const char *endpoint) {
    CoolBoxClient *client = (CoolBoxClient *)malloc(sizeof(CoolBoxClient));
    const char *resolved_endpoint = endpoint != NULL ? endpoint : COOLBOX_C_DEFAULT_ENDPOINT;

    if (client == NULL) {
        return NULL;
    }

    client->endpoint = coolbox_c_dup_string(resolved_endpoint);
    if (client->endpoint == NULL) {
        free(client);
        return NULL;
    }

    return client;
}

void coolbox_c_destroy_client(CoolBoxClient *client) {
    if (client == NULL) {
        return;
    }

    free(client->endpoint);
    free(client);
}

const char *coolbox_c_version(void) {
    return get_metadata_management_library_version();
}

const char *coolbox_c_describe(void) {
    static char description[256];

    snprintf(
        description,
        sizeof(description),
        "CoolBox C bindings linked to %s %s by %s: %s",
        get_metadata_management_library_name(),
        get_metadata_management_library_version(),
        get_metadata_management_library_author(),
        get_metadata_management_library_description()
    );

    return description;
}

size_t coolbox_c_capability_count(void) {
    return sizeof(COOLBOX_C_CAPABILITIES) / sizeof(COOLBOX_C_CAPABILITIES[0]);
}

const char *coolbox_c_capability_at(size_t index) {
    if (index >= coolbox_c_capability_count()) {
        return 0;
    }

    return COOLBOX_C_CAPABILITIES[index];
}

int coolbox_c_is_ready(void) {
    return get_metadata_management_library_version() != NULL;
}

const char *coolbox_c_client_endpoint(const CoolBoxClient *client) {
    if (client == NULL || client->endpoint == NULL) {
        return COOLBOX_C_DEFAULT_ENDPOINT;
    }

    return client->endpoint;
}

const char *coolbox_c_client_version(const CoolBoxClient *client) {
    (void)client;
    return coolbox_c_version();
}

const char *coolbox_c_client_describe(const CoolBoxClient *client) {
    (void)client;
    return coolbox_c_describe();
}

size_t coolbox_c_client_capability_count(const CoolBoxClient *client) {
    (void)client;
    return coolbox_c_capability_count();
}

const char *coolbox_c_client_capability_at(const CoolBoxClient *client, size_t index) {
    (void)client;
    return coolbox_c_capability_at(index);
}

int coolbox_c_client_is_ready(const CoolBoxClient *client) {
    (void)client;
    return coolbox_c_is_ready();
}
