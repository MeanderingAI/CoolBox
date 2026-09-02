module coolbox @test;

import std::core::test;

fn void default_client_has_expected_metadata()
{
	Client client = create_default();

	test::eq(client.endpoint, "local://coolbox");
	assert(client.is_ready());
	test::eq(client.version(), "1.0.0");
	test::eq(client.capability_count(), 3);
	test::eq(client.capability_at(0), "metadata");
	test::eq(client.capability_at(2), "metadata_management");
	assert(client.capabilities().len == 3);
	test::eq(client.capability_at(99), "");
	assert(client.describe().len > 0);
}