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

### Options

| Flag | Default | Description |
|---|---|---|
| `-a, --agent` | `false` | Run as agent instead of server |
| `-t, --token <string>` | `""` | Authorization token (required for agent) |
| `-p, --passive` | `false` | Start server without spawning an agent |

## Data storage

Activity entries are stored by the server under the platform data directory, typically:

| Platform | Path |
|---|---|
| Linux | `~/.local/share/kidmon/reports/` |
| macOS | `~/Library/Application Support/kidmon/reports/` |
| Windows | `%APPDATA%\kidmon\reports\` |

Use [kidmon-reports](../kidmon-reports/README.md) to query this data.

## macOS setup

Install as a launchd service so it starts automatically on login:

```bash
./scripts/install-kidmon-macos.sh
./scripts/uninstall-kidmon-macos.sh   # to remove
```

## Windows service

Register as a Windows service for automatic startup:

```bat
sc create kidmon binPath= "<absolute path to kidmon.exe>" start= auto DisplayName= "Kidmon"
```

## Logs

Log files are written next to the data directory:

- Server: `kidmon-server-YYYY-MM-DD.log`
- Agent: `kidmon-agent-<username>-YYYY-MM-DD.log`

## Message protocol

Agent and server communicate over TCP using JSON messages serialized with [glaze](https://github.com/stephenberry/glaze).

```json
// Agent → Server
{"name": "auth",      "message": {"username": "alice", "token": "secret"}}
{"name": "data",      "message": { ... window and process entry ... }}
{"name": "heartbeat", "message": {"up_time_ms": 12345, "last_activity_time_ms": 12300}}

// Server → Agent
{"status": 0, "error": "", "answer": {}}
```
