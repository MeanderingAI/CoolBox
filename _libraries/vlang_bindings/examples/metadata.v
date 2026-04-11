import coolbox

fn main() {
	client := coolbox.create_default()
	println('endpoint=${client.endpoint}')
	println('ready=${client.is_ready()}')
	println('version=${client.version()}')
	println('description=${client.describe()}')
	for index, capability in client.capabilities() {
		println('capability[${index}]=${capability}')
	}
}