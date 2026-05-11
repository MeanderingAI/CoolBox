module coolbox

fn test_default_client_has_expected_metadata() {
	client := create_default()
	assert client.endpoint == 'local://coolbox'
	assert client.is_ready()
	assert client.version() == '1.0.0'
	assert client.capabilities().len == 3
	assert client.capability_at(0) == 'metadata'
	assert client.capability_at(2) == 'metadata_management'
	assert client.capability_at(99) == ''
	assert client.describe().len > 0
}