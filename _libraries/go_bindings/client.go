package coolboxgo

/*
#cgo CPPFLAGS: -I${SRCDIR}/../c_bindings/include
#cgo windows LDFLAGS: -L${SRCDIR}/../c_bindings/build -L${SRCDIR}/../c_bindings/build/Release -lcoolbox_c_bindings
#cgo darwin LDFLAGS: -L${SRCDIR}/../c_bindings/build -lcoolbox_c_bindings
#cgo linux LDFLAGS: -L${SRCDIR}/../c_bindings/build -lcoolbox_c_bindings
#include "coolbox/coolbox_c.h"
*/
import "C"

const DefaultEndpoint = "local://coolbox"

type Client struct {
	endpoint string
}

func CreateDefaultClient() Client {
	return Client{endpoint: DefaultEndpoint}
}

func ForEndpoint(endpoint string) Client {
	if endpoint == "" {
		endpoint = DefaultEndpoint
	}
	return Client{endpoint: endpoint}
}

func (client Client) Endpoint() string {
	if client.endpoint == "" {
		return DefaultEndpoint
	}
	return client.endpoint
}

func (client Client) Version() string {
	_ = client
	return C.GoString(C.coolbox_c_version())
}

func (client Client) Describe() string {
	_ = client
	return C.GoString(C.coolbox_c_describe())
}

func (client Client) IsReady() bool {
	_ = client
	return C.coolbox_c_is_ready() != 0
}

func (client Client) CapabilityAt(index int) string {
	_ = client
	if index < 0 || index >= client.CapabilityCount() {
		return ""
	}
	value := C.coolbox_c_capability_at(C.size_t(index))
	if value == nil {
		return ""
	}
	return C.GoString(value)
}

func (client Client) CapabilityCount() int {
	_ = client
	return int(C.coolbox_c_capability_count())
}

func (client Client) Capabilities() []string {
	count := client.CapabilityCount()
	capabilities := make([]string, count)
	for index := 0; index < count; index++ {
		capabilities[index] = client.CapabilityAt(index)
	}
	return capabilities
}