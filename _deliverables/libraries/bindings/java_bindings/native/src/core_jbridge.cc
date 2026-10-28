#include "coolbox_jbridge.h"

#include <cstdlib>

extern "C" {

void coolbox_java_free(void* pointer) {
    std::free(pointer);
}

const char* coolbox_java_bridge_version(void) {
    return "1.0.0";
}

}  // extern "C"
