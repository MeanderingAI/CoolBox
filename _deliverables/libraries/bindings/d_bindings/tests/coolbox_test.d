module coolbox_test;

import std.exception : enforce;
import coolbox;

unittest {
    const client = createDefault();

    enforce(client.endpoint == "local://coolbox");
    enforce(client.isReady());
    enforce(client.version().length > 0);
    enforce(client.describe().length > 0);
    enforce(client.capabilityCount() >= 3);
    enforce(client.capabilityAt(0).length > 0);
    enforce(client.capabilityAt(9999) == "");
    enforce(client.capabilities().length == client.capabilityCount());
}
