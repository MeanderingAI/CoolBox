package coolboxgo

import "testing"

func TestMetadataClientSurface(t *testing.T) {
	client := CreateDefaultClient()

	if client.Endpoint() != DefaultEndpoint {
		t.Fatalf("unexpected endpoint: %s", client.Endpoint())
	}
	if !client.IsReady() {
		t.Fatal("client should be ready")
	}
	if client.Version() != "1.0.0" {
		t.Fatalf("unexpected version: %s", client.Version())
	}
	if client.CapabilityCount() != 3 {
		t.Fatalf("unexpected capability count: %d", client.CapabilityCount())
	}
	if client.CapabilityAt(2) != "metadata_management" {
		t.Fatalf("unexpected capability: %s", client.CapabilityAt(2))
	}
	if client.CapabilityAt(99) != "" {
		t.Fatal("out-of-range capability should be empty")
	}
	if client.Describe() == "" {
		t.Fatal("description should not be empty")
	}
}