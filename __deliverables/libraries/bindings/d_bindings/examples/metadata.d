module metadata_example;

import std.stdio : writeln;
import coolbox;

void main() {
    const client = createDefault();

    writeln("endpoint=", client.endpoint);
    writeln("ready=", client.isReady());
    writeln("version=", client.version());
    writeln("description=", client.describe());

    foreach (index, capability; client.capabilities()) {
        writeln("capability[", index, "]=", capability);
    }
}
