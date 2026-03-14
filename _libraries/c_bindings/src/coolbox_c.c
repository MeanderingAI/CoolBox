#include "coolbox/coolbox_c.h"

static const char *const COOLBOX_C_CAPABILITIES[] = {
    "metadata",
    "version",
    "plain-c-sdk-scaffold"
};

const char *coolbox_c_version(void) {
    return "0.1.0";
}

const char *coolbox_c_describe(void) {
    return "CoolBox C bindings ready for plain C integrations.";
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
    return 1;
}
