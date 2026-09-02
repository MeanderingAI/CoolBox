module coolbox_example;

import coolbox;
import std::io;

fn void main()
{
	coolbox::Client client = coolbox::create_default();

	io::printfn("endpoint=%s", client.endpoint);
	io::printfn("ready=%s", client.is_ready() ? "true" : "false");
	io::printfn("version=%s", client.version());
	io::printfn("description=%s", client.describe());

	for (usz i = 0; i < client.capability_count(); i++)
	{
		io::printfn("capability[%d]=%s", i, client.capability_at(i));
	}
}