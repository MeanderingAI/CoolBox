package coolbox

/*
#include <stdlib.h>
#include "coolbox/coolbox_c.h"
*/
import "C"

import (
	"errors"
	"unsafe"
)

const defaultEndpoint = "local://coolbox"

// Client provides access to the metadata and capability APIs.
type Client struct {
	handle *C.CoolBoxClient
}

// DefaultEndpoint returns the endpoint used when a client is created without one.
func DefaultEndpoint() string {
	return defaultEndpoint
}

// NewClient creates a client using the default endpoint.
func NewClient() (*Client, error) {
	handle := C.coolbox_c_create_default_client()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create client")
	}
	return &Client{handle: handle}, nil
}

// NewClientForEndpoint creates a client for endpoint. An empty endpoint uses the default.
func NewClientForEndpoint(endpoint string) (*Client, error) {
	if endpoint == "" {
		endpoint = defaultEndpoint
	}
	value, err := cStringInput(endpoint)
	if err != nil {
		return nil, err
	}
	defer C.free(unsafe.Pointer(value))

	handle := C.coolbox_c_create_client(value)
	if handle == nil {
		return nil, errors.New("coolbox: failed to create client")
	}
	return &Client{handle: handle}, nil
}

// Close releases the native client. It is safe to call Close more than once.
func (client *Client) Close() error {
	if client == nil || client.handle == nil {
		return nil
	}
	C.coolbox_c_destroy_client(client.handle)
	client.handle = nil
	return nil
}

// Endpoint returns the endpoint configured for this client.
func (client *Client) Endpoint() (string, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return "", err
	}
	return C.GoString(C.coolbox_c_client_endpoint(handle)), nil
}

// Version returns the linked metadata library version.
func (client *Client) Version() (string, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return "", err
	}
	return C.GoString(C.coolbox_c_client_version(handle)), nil
}

// Describe returns a description of the linked metadata library.
func (client *Client) Describe() (string, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return "", err
	}
	return C.GoString(C.coolbox_c_client_describe(handle)), nil
}

// IsReady reports whether the native metadata library is ready.
func (client *Client) IsReady() (bool, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_c_client_is_ready(handle) != 0, nil
}

// CapabilityCount returns the number of advertised capabilities.
func (client *Client) CapabilityCount() (uint, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return 0, err
	}
	return uint(C.coolbox_c_client_capability_count(handle)), nil
}

// CapabilityAt returns the capability at index, or an empty string if out of range.
func (client *Client) CapabilityAt(index uint) (string, error) {
	handle, err := client.nativeHandle()
	if err != nil {
		return "", err
	}
	value := C.coolbox_c_client_capability_at(handle, C.size_t(index))
	if value == nil {
		return "", nil
	}
	return C.GoString(value), nil
}

// Capabilities returns all advertised capability names.
func (client *Client) Capabilities() ([]string, error) {
	count, err := client.CapabilityCount()
	if err != nil {
		return nil, err
	}
	capabilities := make([]string, 0, count)
	for index := uint(0); index < count; index++ {
		capability, err := client.CapabilityAt(index)
		if err != nil {
			return nil, err
		}
		capabilities = append(capabilities, capability)
	}
	return capabilities, nil
}

func (client *Client) nativeHandle() (*C.CoolBoxClient, error) {
	if client == nil || client.handle == nil {
		return nil, errors.New("coolbox: client is closed")
	}
	return client.handle, nil
}
