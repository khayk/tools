# kidmon

Distributed process and window activity monitor. A single binary that runs as either a **server** or an **agent** depending on flags passed at startup.

## Architecture

```
[Agent]  ── TCP:51097 ──►  [Server]
  │                          │. │
  │  auth + heartbeat        |  |
  └───────────────────────────. │ persists entries
    window/process data         │
                                ▼
                           FileSystemRepository
```

- The **server** listens on port 51097, spawns an agent process, and persists incoming activity data to disk.
- The **agent** connects to the server, authenticates with a token, then continuously samples the active window and process, and sends the data as structured JSON messages.
- When the agent disconnects, the server respawns it.

Platform-specific window and process sampling lives in `src/os/` and has implementations for Windows, macOS, and Linux.

## Running

Both modes are the same binary. The server spawns the agent automatically unless `--passive` is used.

### Server (default mode)

```bash
kidmon
```

Starts the server. It will spawn an agent process in the background automatically.

```bash
kidmon --passive
```

Starts the server without spawning an agent. Useful when you want to manage the agent lifecycle manually.

### Agent (standalone)

```bash
kidmon --agent --token <auth-token>
```

Connects to a running server and begins monitoring. The token must match what the server expects.

The token may also be supplied via the `KIDMON_TOKEN` environment variable
instead of `--token` (see [Authorization token](#authorization-token) below).
An explicit `--token` takes precedence when both are present.

### Options

| Flag | Default | Description |
|---|---|---|
| `-a, --agent` | `false` | Run as agent instead of server |
| `-t, --token <string>` | `""` | Authorization token. Falls back to `$KIDMON_TOKEN` when omitted |
| `-p, --passive` | `false` | Start server without spawning an agent |

### Authorization token

The server and agent share a token that authorizes the agent's data channel
(see [Message protocol](#message-protocol)).

When the server spawns the agent automatically, it generates a fresh random
token and passes it to the agent through the **`KIDMON_TOKEN` environment
variable** — *not* as a command-line argument. This keeps the secret out of the
process list (`ps`, Task Manager, `/proc/<pid>/cmdline`), where it would
otherwise be readable by other users on the machine. The agent reads
`KIDMON_TOKEN` once and clears it from its own environment immediately.

For manual runs you can still pass the token explicitly:

```bash
# equivalent ways to give the agent its token
kidmon --agent --token <auth-token>
KIDMON_TOKEN=<auth-token> kidmon --agent
```

> The token only protects the loopback channel against *other local users*; a
> process running as the **same user** as the agent can still read it from
> process memory. Hardening the channel against same-user tampering (peer-uid /
> code-identity checks) is tracked separately.

## Data storage

Activity entries are stored by the server under the platform data directory, typically:

| Platform | Path |
|---|---|
| Linux | `~/.local/share/kidmon/reports/` |
| macOS | `~/Library/Application Support/kidmon/reports/` |
| Windows | `%APPDATA%\kidmon\reports\` |

Use [kidmon-reports](../kidmon-reports/README.md) to query this data.

## macOS setup

> **Installing a downloaded release?** Release artifacts are **not** code-signed
> or notarized, so Gatekeeper quarantines them. Clear the flag before first run:
>
> ```bash
> xattr -dr com.apple.quarantine /path/to/kidmon-app.app
> ```
>
> The signing steps below are only for **building locally** — the self-signed
> certificate is a per-machine developer convenience and is never part of CI or
> a release.

On macOS the build produces a signed **application bundle**
(`kidmon-app.app`) rather than a bare binary. This is required so the agent can
capture **window titles**: macOS only populates `kCGWindowName` for a process
that holds **Screen Recording** permission, and that permission can only be
granted to (and remembered for) a code-signed app with a stable identity — not
a bare CLI binary.

### 1. Create a code-signing identity (one time)

```bash
./scripts/create-signing-cert-macos.sh
```

This creates a self-signed `Kidmon Self-Signed` certificate in your login
keychain. A *stable* signature is what lets macOS remember the Screen Recording
grant across rebuilds — TCC keys its database on the code-signing designated
requirement, not on the binary's hash. The script asks for your login password
to authorize `codesign` to use the key without prompting on every build.

> If the script can't create the certificate, make one manually via
> **Keychain Access → Certificate Assistant → Create a Certificate** (Identity
> Type: *Self Signed Root*, Certificate Type: *Code Signing*) named
> `Kidmon Self-Signed`.

The CMake build signs `kidmon-app.app` automatically on every build. If the
certificate is missing the build still succeeds but logs a warning and the
bundle stays unsigned (window titles will be empty). The identity name and
bundle id are configurable via the `KIDMON_CODESIGN_IDENTITY` and
`KIDMON_BUNDLE_ID` CMake cache variables.

### 2. Install as a launchd service

```bash
./scripts/install-kidmon-macos.sh <path-to>/kidmon-app.app agent [--token <token>]
./scripts/uninstall-kidmon-macos.sh agent          # to remove
```

Pass either the `.app` bundle or the inner executable. Installing as a
**LaunchAgent** (rather than running the binary from a terminal) is what makes
macOS attribute the Screen Recording request to **Kidmon** itself instead of
your terminal app. Use `daemon` instead of `agent` to install system-wide
(requires `sudo`).

### 3. Grant Screen Recording permission

On first run the agent triggers the Screen Recording prompt. Approve it, then
**restart the service** so the grant takes effect:

```bash
launchctl stop  com.kidmon.server
launchctl start com.kidmon.server
```

After that, **Kidmon** appears in **System Settings → Privacy & Security →
Screen Recording** and window titles are captured. Because the server and the
spawned agent share one code signature, this single grant covers both.

> Screenshot capture (`WindowImpl::capture`) is not yet implemented on macOS —
> it requires ScreenCaptureKit (also gated behind Screen Recording).

## Windows service

Register as a Windows service for automatic startup:

```bat
sc create kidmon binPath= "<absolute path to kidmon.exe>" start= auto DisplayName= "Kidmon"
```

## Logs

Log files are written next to the data directory:

- Server: `kidmon-server-YYYY-MM-DD.log`
- Agent: `kidmon-agent-<username>-YYYY-MM-DD.log`

## Idle detection

A child often leaves a window in the foreground while away from the keyboard. To
avoid counting that as active time, the agent classifies each cycle as active or
away:

- **Active**: the agent sends a `data` message — the window is recorded and its
  `duration` counts toward time-on-task.
- **Away**: the agent sends a `heartbeat` instead. Nothing is recorded, so idle
  time never inflates the figures. The heartbeat keeps the TCP connection alive,
  so the server does not drop the agent for inactivity while the user is away.

A cycle counts as **away** only when **both**:

1. there has been no keyboard/mouse input for longer than the threshold
   (`Config::idleThreshold`, default 60s), **and**
2. the display has gone to sleep.

The second condition keeps **passive presence counted** — reading, watching a
movie or being in a video call produces no input, but the screen stays on, so
the user is correctly treated as present rather than away. Once the user truly
walks off and the OS display-sleep timeout blanks the screen, time stops
counting.

Detection is platform-specific:

| Signal | Windows | macOS | Linux |
|---|---|---|---|
| Input idle | `GetLastInputInfo` | `CGEventSourceSecondsSinceLastEventType` | not implemented (always active) |
| Display on | `CallNtPowerInformation` (`ES_DISPLAY_REQUIRED`, best-effort) | `CGDisplayIsAsleep` | not implemented (always on) |

On Linux both are unimplemented (the user is always treated as active),
consistent with the not-yet-implemented Linux window backend.

## Message protocol

Agent and server communicate over TCP using JSON messages serialized with
[nlohmann/json](https://github.com/nlohmann/json). The same `toJson`/`fromJson`
functions encode the `Entry` payload on the wire and persist it to the raw data
files. Persisted entries are read back with [glaze](https://github.com/stephenberry/glaze),
whose compile-time reflection makes the report read path fast; its on-disk
schema is keyed on the same `km::constants` strings as the writer, so the two
cannot drift.

```json
// Agent → Server
{"name": "auth",      "message": {"username": "alice", "token": "secret"}}
{"name": "data",      "message": { ... window and process entry ... }}
{"name": "heartbeat", "message": {"up_time_ms": 12345, "last_activity_time_ms": 12300}}

// Server → Agent
{"status": 0, "error": "", "answer": {}}
```

`up_time_ms` is the agent's process uptime; `last_activity_time_ms` is the epoch
timestamp (ms) of the last detected user input. Every agent message is answered
with a status response: `status` is `0` on success and non-zero on failure (with
a human-readable `error`).
