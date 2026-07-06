const std = @import("std");
const c = @cImport({
    @cInclude("coolbox/coolbox_c.h");
});

pub const Client = struct {
    endpoint: []const u8,

    pub fn version(_: Client) []const u8 {
        return cstrToSlice(c.coolbox_c_version());
    }

    pub fn describe(_: Client) []const u8 {
        return cstrToSlice(c.coolbox_c_describe());
    }

    pub fn isReady(_: Client) bool {
        return c.coolbox_c_is_ready() != 0;
    }

    pub fn capabilityCount(_: Client) usize {
        return @as(usize, @intCast(c.coolbox_c_capability_count()));
    }

    pub fn capabilityAt(self: Client, index: usize) []const u8 {
        if (index >= self.capabilityCount()) {
            return "";
        }
        return cstrToSlice(c.coolbox_c_capability_at(@as(c.size_t, @intCast(index))));
    }

    pub fn capabilities(self: Client, allocator: std.mem.Allocator) ![]const []const u8 {
        const count = self.capabilityCount();
        var values = try allocator.alloc([]const u8, count);
        for (values, 0..) |*value, i| {
            value.* = self.capabilityAt(i);
        }
        return values;
    }
};

pub fn createDefault() Client {
    return Client{ .endpoint = "local://coolbox" };
}

pub fn forEndpoint(endpoint: []const u8) Client {
    return Client{ .endpoint = endpoint };
}

fn cstrToSlice(value: ?[*:0]const u8) []const u8 {
    if (value) |ptr| {
        return std.mem.span(ptr);
    }
    return "";
}
