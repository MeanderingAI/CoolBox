#include "coolbox/coolbox_c.h"

#include <assert.h>
#include <string.h>

int main(void) {
    CoolBoxClient *client = coolbox_c_create_default_client();

    assert(client != NULL);
    assert(strcmp(coolbox_c_default_endpoint(), "local://coolbox") == 0);
    assert(strcmp(coolbox_c_client_endpoint(client), "local://coolbox") == 0);
    assert(coolbox_c_client_is_ready(client) == 1);
    assert(strcmp(coolbox_c_client_version(client), "1.0.0") == 0);
    assert(coolbox_c_client_capability_count(client) == 3U);
    assert(strcmp(coolbox_c_client_capability_at(client, 2), "metadata_management") == 0);

    assert(coolbox_c_is_ready() == 1);
    assert(strcmp(coolbox_c_version(), "1.0.0") == 0);
    assert(strstr(coolbox_c_describe(), "metadata_management") != NULL);
    assert(coolbox_c_capability_count() == 3U);
    assert(strcmp(coolbox_c_capability_at(0), "metadata") == 0);
    assert(strcmp(coolbox_c_capability_at(2), "metadata_management") == 0);
    assert(coolbox_c_capability_at(99) == NULL);
    coolbox_c_destroy_client(client);
    return 0;
}
