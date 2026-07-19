# WslDock — Agent Guide

## Commands

```sh
task check       # zig build compile-flags → clang-format -i → clang-tidy (warnings = errors)
task build        # check + zig build (ReleaseSmall, strip, LTO) → zig-out/bin/WslDock-windows-x86_64.exe
task run          # build + launch via cmd /c start
task build-flags  # regenerate compile_flags.txt for IDE support
task release VERSION=x.y.z  # bump version, commit, build, gh release create (requires release_title.txt + release_notes.md)
```

Raw: `mise x -- zig build`, `mise x -- zig build compile-flags`. Tools managed via `mise.toml`.

## Conventions

- **Naming** (`.clang-tidy`, `WarningsAsErrors: *`): functions/vars `camelBack`, types `CamelCase`, constants/macros `UPPER_CASE`
- **Format** (`.clang-format`): LLVM, 120 cols, 4-space indent, attach braces
- **Source patterns**: `#pragma once`, wide chars everywhere, own header first, `// ----` section blocks, `// NOLINT` / `// ReSharper disable` suppressions, `BOOL` returns + `GetLastError()` on failure
- **Gotcha**: `main.c:198,203` — `printf("waiting\n")` / `printf("handle: %lu\n")` debug spew present in production build

## Stack

| Layer | Choice |
|---|---|
| Language | C23 |
| Compiler | Zig (C cross-compiler toolchain) |
| Platform API | Win32 (`user32`, `shell32`, `kernel32`, `gdi32`, `ole32`, `advapi32`) |
| Build | `build.zig` + `Taskfile.yml` |
| Tools | `mise` (zig, bun, gh, cmake, ninja) |
| Icons | `icons/` — `inactive.ico`, `active.ico`, `active_ssh.ico` (multi-res) |
| Config | `%LOCALAPPDATA%\WslDock\wsl_dock.conf` `<state> <instance>` |
