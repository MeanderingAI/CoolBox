package coolbox

/*
#include <stdlib.h>
#include "coolbox/coolbox_c.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

// UUIDV1 generates a time-based UUID (version 1).
func UUIDV1() string {
	return cString(C.coolbox_c_uuid_v1())
}

// UUIDV2 generates a DCE Security UUID (version 2).
func UUIDV2(localIdentifier, localDomain uint32) string {
	return cString(C.coolbox_c_uuid_v2(C.uint(localIdentifier), C.uint(localDomain)))
}

// UUIDV3 generates a name-based UUID using MD5.
func UUIDV3(namespace, name string) (string, error) {
	namespaceValue, err := cStringInput(namespace)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(namespaceValue))

	nameValue, err := cStringInput(name)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(nameValue))
	return cString(C.coolbox_c_uuid_v3(namespaceValue, nameValue)), nil
}

// UUIDV4 generates a random UUID (version 4).
func UUIDV4() string {
	return cString(C.coolbox_c_uuid_v4())
}

// UUIDV5 generates a name-based UUID using SHA-1.
func UUIDV5(namespace, name string) (string, error) {
	namespaceValue, err := cStringInput(namespace)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(namespaceValue))

	nameValue, err := cStringInput(name)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(nameValue))
	return cString(C.coolbox_c_uuid_v5(namespaceValue, nameValue)), nil
}

// UUIDV6 generates a reordered time-based UUID (version 6).
func UUIDV6() string {
	return cString(C.coolbox_c_uuid_v6())
}

// UUIDV8 generates a custom UUID from hexadecimal entropy.
func UUIDV8(entropyHex string) (string, error) {
	value, err := cStringInput(entropyHex)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(value))
	return cString(C.coolbox_c_uuid_v8(value)), nil
}

// GUID generates a GUID using the native CoolBox UUID implementation.
func GUID() string {
	return cString(C.coolbox_c_guid())
}

// CUID generates a CUID using the native CoolBox UUID implementation.
func CUID() string {
	return cString(C.coolbox_c_cuid())
}

func cString(value *C.char) string {
	if value == nil {
		return ""
	}
	return C.GoString(value)
}

func cStringInput(value string) (*C.char, error) {
	result := C.CString(value)
	if result == nil {
		return nil, fmt.Errorf("coolbox: failed to allocate native string")
	}
	if C.GoString(result) != value {
		C.free(unsafe.Pointer(result))
		return nil, fmt.Errorf("coolbox: value contains an embedded NUL byte")
	}
	return result, nil
}
