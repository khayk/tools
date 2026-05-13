# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test

**Prerequisites:** `VCPKG_ROOT` env var must point to a vcpkg installation. Requires `ninja` and `pkg-config`.

### Configure & Build

Using CMake presets (preferred):
```bash
cmake --preset vcpkg-debug          # configure (debug into build/debug/)
cmake --build --preset vcpkg-debug  # build
```

Without presets:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 8 --config Debug
```

Available presets: `vcpkg-debug`, `vcpkg-release`, `vcpkg-gcc15-debug`, `vcpkg-clang-debug`, `vcpkg-coverage`.

Enable clang-tidy checks: add `-DTOOLS_TIDY=ON` to configure step.

### Tests

```bash
ctest --test-dir build/debug                        # run all tests
ctest --test-dir build/debug -R ".*Pattern.*"       # filter by regex
ctest --test-dir build/debug -N                     # list tests without running
```

Run a single test binary directly:
```bash
./build/debug/core/test/core-test
./build/debug/duplicates/test/duplicates-test
./build/debug/kidmon/test/kidmon-test
```

### Coverage (requires `lcov` and `genhtml`)

```bash
cmake --preset vcpkg-coverage                              # configure (into build/coverage/)
cmake --build --preset vcpkg-coverage                      # build all targets with instrumentation
cmake --build --preset vcpkg-coverage -t coverage-core-test       # generate core report
cmake --build --preset vcpkg-coverage -t coverage-duplicates-test # generate duplicates report
cmake --build --preset vcpkg-coverage -t coverage-kidmon-test     # generate kidmon report
# HTML reports appear in build/coverage/coverage-<target-name>/
```

### Formatting

```bash
find . -type f \( -name "*.cpp" -o -name "*.h" \) -not -path "./build/*" | xargs clang-format -i
```

Requires `clang-format` >= 17.0.1. Style rules are in [.clang-format](.clang-format).

## Architecture

This is a **C++23 monorepo** with four tools built on a shared core library. Dependency management via vcpkg; static analysis via clang-tidy (config in [.clang-tidy](.clang-tidy)).

### Projects

| Project | Purpose |
|---|---|
| `core/` | Shared library: TCP networking, Observer/Callback/Singleton patterns, crypto, logging |
| `duplicates/` | Finds and deletes duplicate files using SHA256 hashing |
| `kidmon/` | Distributed process/window activity monitor (server + agent) |
| `kidmon-reports/` | Transforms and filters kidmon data for reporting |

Each project follows the layout: `include/`, `src/`, `test/`, optionally `doc/`.

### Core Library

- **TCP stack** (`core/include/core/network/`): Asio-based `Server`, `Client`, `Connection`, `Communicator`. Connections accept a factory lambda for socket creation and an error callback.
- **Packer/Unpacker** (`core/include/core/network/data/`): Serializes messages over TCP via an `ISource` interface.
- **Design patterns** (`core/include/core/patterns/`): `Observable<T>` (multi-arg notifier), `ScopedObserve<T>` (RAII observer), `Callback<T>`, `Singleton<T>`.
- **Utilities**: `spdlog` logging, OpenSSL crypto, file I/O helpers, string ops.

### Kidmon Architecture

Server and agent communicate over TCP on port **51097** using token-based authorization. The flow:

1. **Server**: listens → accepts connection → waits for auth message → routes data messages via `MsgHandler`
2. **Agent**: connects → sends `{username, token}` auth → collects window/process data periodically → sends heartbeat → reconnects on disconnect

Platform-specific implementations live in `kidmon/src/os/` (Windows/macOS/Linux). Message serialization uses the `glaze` library. Entry data is persisted via a repository pattern (`FileSystemRepository`).

### Namespaces

- `tcp::` — networking
- `dp::` — design patterns
- `tools::dups::` — duplicates tool
- Project-specific namespaces per tool

### Key Conventions

- **C++ standard:** C++23
- **Headers:** `#pragma once`
- **Ownership:** `std::unique_ptr`/`std::shared_ptr`; no raw ownership transfer
- **Logging:** `spdlog` with `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE`
- **Config files:** TOML (duplicates), JSON (kidmon)
- **Column limit:** 87 characters (enforced by clang-format)
- **Namespace alias:** `namespace fs = std::filesystem` is the project convention
