module coolbox;

extern fn ZString coolbox_c_version();
extern fn ZString coolbox_c_describe();
extern fn usz coolbox_c_capability_count();
extern fn ZString coolbox_c_capability_at(usz index);
extern fn int coolbox_c_is_ready();

struct Client
{
	String endpoint;
}

fn Client create_default()
{
	return { .endpoint = "local://coolbox" };
}

fn Client for_endpoint(String endpoint)
{
	return { .endpoint = endpoint };
}

fn String Client.version(&self)
{
	ZString value = coolbox_c_version();
	return value == null ? "" : value.str_view();
}

fn String Client.describe(&self)
{
	ZString value = coolbox_c_describe();
	return value == null ? "" : value.str_view();
}

fn bool Client.is_ready(&self)
{
	return coolbox_c_is_ready() != 0;
}

fn usz Client.capability_count(&self)
{
	return coolbox_c_capability_count();
}

fn String Client.capability_at(&self, usz index)
{
	ZString value = coolbox_c_capability_at(index);
	return value == null ? "" : value.str_view();
}

fn String[] Client.capabilities(&self)
{
	usz count = self.capability_count();
	String[] values = mem::alloc_array(String, count);
	for (usz i = 0; i < count; i++)
	{
		values[i] = self.capability_at(i);
	}
	return values;
}