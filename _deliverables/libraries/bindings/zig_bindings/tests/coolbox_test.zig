const std = @import("std");
const testing = std.testing;
const coolbox = @import("coolbox");

test "default client has expected metadata" {
    const client = coolbox.createDefault();

    try testing.expectEqualStrings("local://coolbox", client.endpoint);
    try testing.expect(client.isReady());
    try testing.expect(client.version().len > 0);
    try testing.expect(client.describe().len > 0);
    try testing.expect(client.capabilityCount() >= 3);
    try testing.expect(client.capabilityAt(0).len > 0);
    try testing.expectEqualStrings("", client.capabilityAt(9999));
}
