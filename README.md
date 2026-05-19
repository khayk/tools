# Tools

[![Linux](https://github.com/khayk/tools/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/khayk/tools/actions/workflows/ci-linux.yml)
[![macOS](https://github.com/khayk/tools/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/khayk/tools/actions/workflows/ci-macos.yml)
[![Windows](https://github.com/khayk/tools/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/khayk/tools/actions/workflows/ci-windows.yml)
[![Coverage](https://github.com/khayk/tools/actions/workflows/coverage.yml/badge.svg?branch=main)](https://github.com/khayk/tools/actions/workflows/coverage.yml)

A C++23 monorepo of personal utility tools built on a shared core library.

## Projects

| Project | Description |
|---|---|
| [core](core/README.md) | Shared library — TCP networking, design patterns, crypto, logging |
| [duplicates](duplicates/README.md) | Finds and removes duplicate files using SHA-256 hashing |
| [kidmon](kidmon/README.md) | Distributed process/window activity monitor (server + agent) |
| [kidmon-reports](kidmon-reports/README.md) | Queries and displays kidmon activity data |

## Requirements

- C++ compiler with C++23 support — GCC 13+, Clang 16+, or MSVC 2022+
- CMake 3.25+
- Ninja and pkg-config
- [vcpkg](https://github.com/microsoft/vcpkg) — clone it, then set `VCPKG_ROOT` to its root directory

Dependencies are declared in [vcpkg.json](vcpkg.json) and installed automatically on first build
    - In case of errors check if these packages are installed `perl-core perl-IPC-Cmd kernel-headers ninja-build`



## Build

```bash
cmake --preset vcpkg-debug          # configure → build/debug/
cmake --build --preset vcpkg-debug  # build
```

Available presets: `vcpkg-debug`, `vcpkg-release`, `vcpkg-gcc15-debug`, `vcpkg-clang-debug`, `vcpkg-coverage`.

Add `-DTOOLS_TIDY=ON` to enable clang-tidy static analysis during configure.

## Tests

```bash
ctest --test-dir build/debug --output-on-failure   # all tests
ctest --test-dir build/debug -R ".*Pattern.*"       # filter by regex
ctest --test-dir build/debug -N                     # list without running
```

## Coverage

Use the helper script (requires `lcov`):

```bash
./scripts/coverage.sh                           # all targets
./scripts/coverage.sh core-test                 # one target
./scripts/coverage.sh core-test duplicates-test
```

HTML reports are written to `build/coverage/coverage-<target>/index.html`. Coverage from the `main` branch is published to GitHub Pages:

**https://khayk.github.io/tools/**

## Formatting

Style rules are in [.clang-format](.clang-format). Requires `clang-format` 17+.

```bash
./scripts/format.sh
```

## Release

Releases are triggered by pushing a version tag. The [release workflow](.github/workflows/release.yml) builds all three platforms, packages the binaries, and creates a GitHub release automatically.

1. Update the version in [CMakeLists.txt](CMakeLists.txt) if needed:
   ```cmake
   project(tools VERSION 0.2.0 LANGUAGES CXX)
   ```
2. Commit, push to `main`, and wait for CI to go green.
3. Tag the commit and push the tag:
   ```bash
   git tag v0.2.0
   git push origin v0.2.0
   ```

The release is published at **https://github.com/khayk/tools/releases**.

## CI

Every push to `main` and every pull request is tested on three platforms, each in its own workflow:

| Workflow | Platform | Compiler |
|---|---|---|
| [ci-linux.yml](.github/workflows/ci-linux.yml) | Ubuntu 24.04 | GCC |
| [ci-macos.yml](.github/workflows/ci-macos.yml) | macOS (latest) | AppleClang |
| [ci-windows.yml](.github/workflows/ci-windows.yml) | Windows 2022 | MSVC |

Coverage reports are generated on push to `main` via [coverage.yml](.github/workflows/coverage.yml) and published to **https://khayk.github.io/tools/**.

---

## Project details

### core

The shared library used by all other projects. Provides an Asio-based TCP stack, reusable design patterns (`Observable`, `Callback`, `Singleton`), OpenSSL crypto helpers, and common utilities for logging, file I/O, and string handling.

[→ core/README.md](core/README.md)

### duplicates

A command-line tool that scans one or more directories for duplicate files (identified by SHA-256 hash) and lets you delete them — either interactively or automatically based on keep/delete path rules. Supports dry-run mode.

[→ duplicates/README.md](duplicates/README.md)

### kidmon

A client-server activity monitor. The server runs on the host machine; the agent runs (spawned by the server or separately) and streams active window and process data over TCP. Data is stored locally and analysed with `kidmon-reports`.

[→ kidmon/README.md](kidmon/README.md)

### kidmon-reports

A query tool for kidmon activity data. Filter by user, time range, process name, or window title. Supports top-N ranking, case-insensitive search, and include/exclude conditions.

[→ kidmon-reports/README.md](kidmon-reports/README.md)

## License

[MIT](LICENSE) © Hayk Karapetyan
