#include "coolbox/coolbox_c.h"

#include <assert.h>
#include <string.h>

int main(void) {
    assert(coolbox_c_is_ready() == 1);
    assert(strcmp(coolbox_c_version(), "0.1.0") == 0);
    assert(strstr(coolbox_c_describe(), "plain C integrations") != NULL);
    assert(coolbox_c_capability_count() == 3U);
    assert(strcmp(coolbox_c_capability_at(0), "metadata") == 0);
    assert(strcmp(coolbox_c_capability_at(2), "plain-c-sdk-scaffold") == 0);
    assert(coolbox_c_capability_at(99) == NULL);
    return 0;
}
