const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSmall });

    const arch_name = if (target.result.cpu.arch == .x86_64) "x86_64" else @tagName(target.result.cpu.arch);
    const exe_name = b.fmt("WslDock-windows-{s}", .{arch_name});

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    mod.addCSourceFiles(.{
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

    mod.addIncludePath(b.path("src"));

    mod.addWin32ResourceFile(.{
        .file = b.path("resource.rc"),
    });

    mod.linkSystemLibrary("user32", .{});
    mod.linkSystemLibrary("shell32", .{});
    mod.linkSystemLibrary("kernel32", .{});
    mod.linkSystemLibrary("gdi32", .{});
    mod.linkSystemLibrary("ole32", .{});
    mod.linkSystemLibrary("advapi32", .{});
    mod.link_libc = true;

    const exe = b.addExecutable(.{
        .name = exe_name,
        .root_module = mod,
    });

    if (optimize == .ReleaseSmall) {
        exe.root_module.strip = true;
        exe.lto = .full;
    }

    exe.subsystem = .windows;

    b.installArtifact(exe);

    const cflags_file = b.step("compile-flags", "Generate compile_flags.txt for C/C++ IDE support");
    const write_cflags = b.addSystemCommand(&.{
        "pwsh", "-NoP", "-C",
        "@('-O2', '-std=c23', '-Wall', '-Wextra', '-DUNICODE', '-D_UNICODE', '-Isrc') | Out-File compile_flags.txt -Encoding ASCII",
    });
    cflags_file.dependOn(&write_cflags.step);
}
