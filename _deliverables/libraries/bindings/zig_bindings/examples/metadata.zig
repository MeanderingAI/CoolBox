const std = @import("std");
const coolbox = @import("coolbox");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    const allocator = gpa.allocator();
    const client = coolbox.createDefault();

    const stdout = std.io.getStdOut().writer();
    try stdout.print("endpoint={s}\n", .{client.endpoint});
    try stdout.print("ready={}\n", .{client.isReady()});
    try stdout.print("version={s}\n", .{client.version()});
    try stdout.print("description={s}\n", .{client.describe()});

    const values = try client.capabilities(allocator);
    defer allocator.free(values);
    for (values, 0..) |capability, i| {
        try stdout.print("capability[{d}]={s}\n", .{ i, capability });
    }
}
