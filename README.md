# WslDock

A minimalist Win32 tray application that keeps WSL instances alive and manages SSH agent tunnels.

Built on the same foundation as [StayAwake](https://github.com/TheFranconianCoder/stay-awake) — same Zig toolchain, same lean Win32 approach, ~1–2 MB footprint.

---

## States & Icons

| State | Icon | Description |
|-------|------|-------------|
| Inactive | `inactive.ico` | WSL runs unmanaged |
| Active | `active.ico` | `wsl -d <instance> sleep infinity` anchors the VM |
| Active + SSH | `active_ssh.ico` | VM anchored + `ssh -A localhost` tunnel |

---

## Usage

**Left-click** tray icon → toggle Inactive ↔ Active

**Right-click** → context menu:

| Item | Behaviour |
|------|-----------|
| `SSH-Tunnel` *(checkmark when on)* | Toggle SSH tunnel. Implicitly activates WSL sleep if not yet running. |
| `Reboot` | `wsl -t <instance>` → restart sleep (+ SSH if it was active) |
| `Exit` | Remove tray icon and quit. **WSL/SSH processes keep running.** |

---

## Configuration

```
%LOCALAPPDATA%\WslDock\wsl_dock.conf
```

Single line, two space-separated values:

```
<state> <instance>
```

| Field | Values |
|-------|--------|
| `state` | `0` = inactive · `1` = active · `2` = active + SSH |
| `instance` | WSL distro name (default: `Default`) |

**Examples:**
```
0 Default
1 Ubuntu-24.04
2 Ubuntu-24.04
```

Written atomically on every state change via temp-file-swap.
If the file does not exist, WslDock starts in the inactive state with `Default`.

Changes to the file are picked up live (no restart needed).

### Set instance name via CLI

```
WslDock.exe Ubuntu-24.04
```

Writes the instance name to config and starts normally. Useful for first-run or scripted deployment via mise.

---

## Requirements

- The target WSL distro must have `sshd` running for the SSH tunnel feature.
- `ssh -A localhost` — key-based auth must be configured inside the distro.

---

## Build

```sh
zig build
```

Place three `.ico` files in `icons/` before building:

| File | State |
|------|-------|
| `icons/inactive.ico` | Inactive |
| `icons/active.ico` | Active |
| `icons/active_ssh.ico` | Active + SSH |

Recommended: multi-resolution ICO (16×16, 32×32, 48×48).

```sh
# With mise + task
mise install
task build
```

---

## Installation via mise

```sh
mise use -g github:TheFranconianCoder/wsl-dock
```

Autostart is registered on every launch via `HKCU\...\Run`, always pointing to the current binary — path updates after a `mise upgrade` are handled automatically.

---

## Release

```sh
task release VERSION=0.2.0
```

Requires `release_title.txt` and `release_notes.md` to exist before running.
