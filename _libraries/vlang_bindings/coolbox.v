module coolbox

#flag windows -I @VMODROOT/../c_bindings/include
#flag linux -I @VMODROOT/../c_bindings/include
#flag darwin -I @VMODROOT/../c_bindings/include

#flag windows -L @VMODROOT/../c_bindings/build
#flag windows -L @VMODROOT/../c_bindings/build/Release
#flag linux -L @VMODROOT/../c_bindings/build
#flag darwin -L @VMODROOT/../c_bindings/build

#flag -lcoolbox_c_bindings

#include "coolbox/coolbox_c.h"

fn C.coolbox_c_version() &char
fn C.coolbox_c_describe() &char
fn C.coolbox_c_capability_count() usize
fn C.coolbox_c_capability_at(index usize) &char
fn C.coolbox_c_is_ready() int

pub struct Client {
pub:
	endpoint string
}

pub fn create_default() Client {
	return Client{
		endpoint: 'local://coolbox'
	}
}

pub fn for_endpoint(endpoint string) Client {
	return Client{
		endpoint: endpoint
	}
}

pub fn (client Client) version() string {
	_ = client
	return unsafe { cstring_to_vstring(C.coolbox_c_version()) }
}

pub fn (client Client) describe() string {
	_ = client
	return unsafe { cstring_to_vstring(C.coolbox_c_describe()) }
}

pub fn (client Client) is_ready() bool {
	_ = client
	return C.coolbox_c_is_ready() != 0
}

pub fn (client Client) capability_at(index int) string {
	_ = client
	if index < 0 || index >= int(C.coolbox_c_capability_count()) {
		return ''
	}
	value := C.coolbox_c_capability_at(usize(index))
	if value == unsafe { nil } {
		return ''
	}
	return unsafe { cstring_to_vstring(value) }
}

pub fn (client Client) capabilities() []string {
	_ = client
	count := int(C.coolbox_c_capability_count())
	mut capabilities := []string{len: count}
	for index in 0 .. count {
		capabilities[index] = unsafe { cstring_to_vstring(C.coolbox_c_capability_at(usize(index))) }
	}
	return capabilities
}