const std = @import("std");
const compile_flagz = @import("compile_flagz");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSmall });

    const arch_name = if (target.result.cpu.arch == .x86_64) "x86_64" else @tagName(target.result.cpu.arch);
    const exe_name = b.fmt("WslDock-windows-{s}", .{arch_name});

    const exe = b.addExecutable(.{
        .name = exe_name,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    if (optimize == .ReleaseSmall) {
        exe.root_module.strip = true;
        exe.want_lto = true;
    }

    exe.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/config.c",
            "src/tray.c",
            "src/wsl.c",
        },
        .flags = &.{
            "-O2",
            "-std=c23",
            "-Wall",
            "-Wextra",
            "-DUNICODE",
            "-D_UNICODE",
        },
    });

    exe.addIncludePath(b.path("src"));

    exe.addWin32ResourceFile(.{
        .file = b.path("resource.rc"),
    });

    exe.linkSystemLibrary("user32");
    exe.linkSystemLibrary("shell32");
    exe.linkSystemLibrary("kernel32");
    exe.linkSystemLibrary("gdi32");
    exe.linkSystemLibrary("ole32");
    exe.linkSystemLibrary("advapi32");
    exe.linkLibC();

    exe.subsystem = .Windows;
    b.installArtifact(exe);

    var cflags = compile_flagz.addCompileFlags(b);
    cflags.addIncludePath(b.path("src"));

    const cflags_step = b.step("compile-flags", "Generate compile_flags.txt for C/C++ IDE support");
    cflags_step.dependOn(&cflags.step);
}
