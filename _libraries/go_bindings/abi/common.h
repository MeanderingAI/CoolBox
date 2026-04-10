#ifndef COOLBOX__LIBRARIES_GO_BINDINGS_ABI_COMMON_H
#define COOLBOX__LIBRARIES_GO_BINDINGS_ABI_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CoolBoxStatus {
    COOLBOX_STATUS_OK = 0,
    COOLBOX_STATUS_INVALID_ARGUMENT = 1,
    COOLBOX_STATUS_FIT_FAILED = 2,
    COOLBOX_STATUS_PREDICT_FAILED = 3,
    COOLBOX_STATUS_INTERNAL_ERROR = 4
} CoolBoxStatus;

#ifdef __cplusplus
}
#endif
#endif  // COOLBOX__LIBRARIES_GO_BINDINGS_ABI_COMMON_H
