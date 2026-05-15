# core

Shared C++23 library used by all other projects in this repo. Provides TCP networking, reusable design patterns, crypto, and general-purpose utilities.

## What's inside

### TCP networking (`core/network/`)

Asio-based stack for building networked applications:

| Class | Role |
|---|---|
| `TcpServer` | Listens for incoming connections; calls a factory lambda to create sockets |
| `TcpClient` | Connects to a server; reconnects on disconnect |
| `TcpConnection` | A single bidirectional connection with an error callback |
| `TcpCommunicator` | Higher-level send/receive wrapper over a connection |
| `Packer` / `Unpacker` | Serializes and deserializes messages over TCP via an `ISource` interface |

### Design patterns (`core/patterns/`)

| Class | Description |
|---|---|
| `Observable<T>` | Multi-argument event notifier; observers register and are called on `notify()` |
| `ScopedObserve<T>` | RAII wrapper that unregisters the observer on destruction |
| `Callback<T>` | Single-slot typed callback |
| `Singleton<T>` | Classic singleton with controlled lifetime |

### Utilities (`core/utils/`)

| Header | Provides |
|---|---|
| `Crypto.h` | SHA-256 hashing via OpenSSL |
| `Log.h` | spdlog-based logger setup (`configureLogger`) |
| `File.h` | File read helpers (`readLines`, `path2s`) |
| `Str.h` | String conversions, `humanizeDuration`, `humanizeBytes`, UTF-8 lowercase |
| `Sys.h` | Active username, interactive-session detection, memory usage |
| `Dirs.h` | Platform-correct config / cache / data / log directories |
| `StopWatch.h` | High-resolution elapsed time |
| `Tracer.h` | RAII scope tracer that logs entry/exit |
| `SingleInstanceChecker.h` | Prevents running more than one copy of a process |
| `Throw.h` | Checked-throw helpers |

## Building

`core` is not built standalone — it is always built as part of the monorepo:

```bash
cmake --preset vcpkg-debug
cmake --build --preset vcpkg-debug
```

## Tests

```bash
ctest --test-dir build/debug -R "core"
# or directly:
./build/debug/core/test/core-test
```

Test source is under [test/](test/) and covers the network stack, design patterns, and utilities.

## Namespaces

- `tcp::` — networking classes
- `dp::` — design pattern classes
- `core::utl::` — logging utilities
- `core::str::` — string utilities
- `core::sys::` — system utilities
- `core::dirs::` — directory helpers
- `core::file::` — file helpers
