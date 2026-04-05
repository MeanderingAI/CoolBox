package coolboxgo

/*
#include <stdlib.h>
*/
import "C"

import (
    "fmt"
    "unsafe"
)

func cError(err *C.char) error {
    if err == nil {
        return nil
    }
    defer C.free(unsafe.Pointer(err))
    return fmt.Errorf("%s", C.GoString(err))
}

func cDoublePtr(values []float64) *C.double {
    if len(values) == 0 {
        return nil
    }
    return (*C.double)(unsafe.Pointer(&values[0]))
}

func cCIntPtr(values []C.int) *C.int {
    if len(values) == 0 {
        return nil
    }
    return (*C.int)(unsafe.Pointer(&values[0]))
}

func toCIntSlice(values []int) []C.int {
	converted := make([]C.int, len(values))
	for i, value := range values {
		converted[i] = C.int(value)
	}
	return converted
}

func fromCIntSlice(values []C.int) []int {
    out := make([]int, len(values))
    for i, value := range values {
        out[i] = int(value)
    }
    return out
}
