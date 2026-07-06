const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const coolbox_mod = b.addModule("coolbox", .{
        .root_source_file = b.path("src/coolbox.zig"),
        .target = target,
        .optimize = optimize,
    });

    const example = b.addExecutable(.{
        .name = "coolbox_metadata",
        .root_source_file = b.path("examples/metadata.zig"),
        .target = target,
        .optimize = optimize,
    });
    example.root_module.addImport("coolbox", coolbox_mod);
    example.addIncludePath(b.path("../c_bindings/include"));
    example.addLibraryPath(b.path("../c_bindings/build"));
    example.addLibraryPath(b.path("../c_bindings/build/Release"));
    example.linkSystemLibrary("coolbox_c_bindings");
    b.installArtifact(example);

    const tests = b.addTest(.{
        .root_source_file = b.path("tests/coolbox_test.zig"),
        .target = target,
        .optimize = optimize,
    });
    tests.root_module.addImport("coolbox", coolbox_mod);
    tests.addIncludePath(b.path("../c_bindings/include"));
    tests.addLibraryPath(b.path("../c_bindings/build"));
    tests.addLibraryPath(b.path("../c_bindings/build/Release"));
    tests.linkSystemLibrary("coolbox_c_bindings");

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run zig binding tests");
    test_step.dependOn(&run_tests.step);
}
