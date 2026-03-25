#include "coolbox/coolbox_c.h"

#include <stdio.h>

static const char *const COOLBOX_C_CAPABILITIES[] = {
    "metadata",
    "version",
    "metadata_management"
};

extern const char *get_metadata_management_library_name(void);
extern const char *get_metadata_management_library_version(void);
extern const char *get_metadata_management_library_description(void);
extern const char *get_metadata_management_library_author(void);

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
