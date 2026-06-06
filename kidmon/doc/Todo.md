# Todo

* Troubleshoot why the kidmon-reports workings so slow (a small amount of data is processed during ~300ms)

## Open Issues

### Critical — security (this is a surveillance daemon; treat it like one)

These matter precisely because the tool can be installed as a **root `daemon`** (per the README) and captures screen contents and window titles.


**C5 — No transport protection beyond "it's localhost."** Screenshots (base64) and window titles cross the loopback socket in cleartext, and _any_ local process can open port 51097. The only gate is the token from C2/C3. The trust model is effectively "any local process that can run `ps`." That may be acceptable for a single-user kid's machine — but it should be a documented, deliberate decision, not an accident. Constant-time token compare ([AuthorizationHandler](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/handler/AuthorizationHandler.cpp#L56) uses `!=`) is a minor footnote next to C3.

---

### Major — correctness & "documented but not implemented"

**M1 — Disk I/O runs on the event-loop thread.** `DataHandler::handle` → `repo_.add` → `file::append` / `file::write` are **blocking** filesystem calls executed directly on the single asio thread ([DataHandler.cpp:49](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/handler/DataHandler.cpp#L49)). Every screenshot write stalls all networking and the health-check timer. Offload persistence to a worker / `post()` to a separate strand+thread, or at minimum a writer queue.

**M2 — The heartbeat protocol is documented but does not exist.** The README and [Docs.md](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/doc/Docs.md) specify a `heartbeat` message with `up_time_ms` / `last_activity_time_ms`, and `GetLastInputInfo` is referenced — but `grep` finds **zero** heartbeat code in the agent. The agent never sends one. Worse, the implied feature — **idle/away detection** — is the thing a "kid activity monitor" most needs: without it, the agent records the same foreground window for hours while the child is away from the keyboard, inflating every "time spent" number. This is a product-level gap, not just a missing message.

**M3 — There is no config file, despite claims otherwise.** [Todo.md](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/doc/Todo.md) marks "Server and agent should have their own config" as _Done_, and CLAUDE.md says config is JSON — but [Config.cpp](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/config/Config.cpp) only computes directories, [Main.cpp](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/Main.cpp) never loads a file, and `takeSnapshots` / `calcSha` / intervals are hardcoded defaults that nothing ever sets. Snapshots and SHA are effectively dead-off. Either implement loading or stop claiming it.

**M4 — Spawn race in the health check.** [healthCheck](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/KidmonServer.cpp#L106) fires every 2s and spawns an agent whenever `!hasAuthorizedAgent()`. Between spawning agent #1 (token T1) and that agent finishing auth, the next tick sees "no authorized agent," spawns agent #2 with token T2, and **overwrites the server token to T2** — now agent #1 can never authenticate. On any slow start you get agent churn / duplicate processes. There's no "spawn pending" state. Track an outstanding-spawn timestamp and don't re-spawn within a grace window.

**M5 — Malformed disk lines are silently turned into bogus entries.** In [readEntries](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/repo/FileSystemRepository.cpp#L177), a parse exception does `entry = Entry()` and then **still falls through to `cb(entry)`**, feeding a default-constructed entry into the query results. It should `return true` (skip) on failure, not emit a zero entry. Relatedly, [queryEntries](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/repo/FileSystemRepository.cpp#L303) does `std::stoi(filename)` on every subdirectory — a single stray non-numeric directory throws and aborts the whole query.

**M6 — `buildResponse` never reports failure status.** [AgentConnection](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/AgentConnection.cpp#L62) always sends `buildResponse(0, ...)` — status 0 even on auth failure (it conveys failure only through `answer.authorized=false`, then closes). The documented `{"status":N,"error":...}` non-zero path is never produced, and the agent's `WaitingAuth` branch keys off `status != 0` that the server never sends. The protocol contract and the implementation disagree.

---

### Design / maintainability

**D1 — Two JSON libraries, three parse paths for one type.** `Entry` is **written** with nlohmann ([Types.cpp toJson](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/data/Types.cpp)), **read off the wire** with nlohmann `fromJson`, and **read off disk** with glaze in [readEntries](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/repo/FileSystemRepository.cpp#L137) — a hand-rolled third extraction keyed on `constants::`. Three representations of the same schema that must be kept in lockstep by hand; they _will_ drift. Pick one library and one serialization function. (The glaze dependency exists almost entirely to power this redundant read path.)

**D2 — `file(GLOB)` for sources.** [CMakeLists.txt](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/CMakeLists.txt#L6) globs sources (and globs a non-existent `src/geometry/`), so adding a file doesn't trigger reconfigure — the canonical CMake anti-pattern. The test [GLOB_RECURSE](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/test/CMakeLists.txt#L3) then _also_ lists `common/*` `repo/*` redundantly (recurse already covers them). List sources explicitly.

**D3 — Wasted work every sample cycle.** [collectData](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/agent/KidmonAgent.cpp#L276) builds an `ostringstream` of the rect and calls `activeUserName()` twice per tick — all unconditionally, even though the rect string only feeds a `debug` log. Guard debug-only formatting; compute the username once.

**D4 — Base64-in-JSON for image bytes over the wire.** Screenshots are base64-encoded ([agent](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/agent/KidmonAgent.cpp#L321)) then embedded in a JSON line — ~33% inflation plus JSON escaping, then decoded server-side. (Credit: the _disk_ format correctly excludes the bytes via `includeImageBytes=false` and writes the image separately.) For binary payloads, a separate binary frame beats base64-in-JSON.

**D5 — Magic port `51097` and intervals duplicated** across `KidmonServer::Config`, `KidmonAgent::Config`, and docs. One source of truth.

**D6 — `Docs.md` is a committed scratchpad** — commented-out cereal code, a stray `*/`, and unrelated "Everything indexer" notes. Delete it or fold the real state machine into the README; right now it actively misleads (it still describes `cereal`, which isn't used).

**D7 — `queryRawDataDir` filename-range logic is hard to follow and likely buggy at year boundaries** ([FileSystemRepository.cpp:186](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/repo/FileSystemRepository.cpp#L186)) — overlapping conditions with `keepGoing` folded into the directory-iterator filter, and the directory is iterated unsorted while the logic assumes ordering. Decompose and unit-test the boundary cases explicitly.

---

### Recommended priority order

|Pri|Item|Why|
|---|---|---|
|**P0**|C1 path traversal; C2 CSPRNG token; C4 frame cap|Active vulnerabilities in a possibly-root daemon|
|**P0**|C3 token off the command line|The auth gate is currently bypassable by any local user|
|**P1**|M1 off-thread disk I/O; M4 spawn race; M5 bogus-entry bug|Stability/correctness of the running system|
|**P1**|M2 idle detection + heartbeat|Without it the core metric (time-on-task) is wrong|
|**P2**|M3 real config; D1 single serialization path; M6 protocol/status consistency|Removes "documented but fake" surfaces and drift risk|
|**P3**|D2 CMake globs; D3 wasted work; D6 docs cleanup; D5 magic numbers|Hygiene|

---

### If I were redesigning the data path

The current flow (agent samples → JSON-over-TCP → server validates → blocking append to per-day raw `.dat`, separate image files) is reasonable for a prototype but has the wrong center of gravity. I'd:

1. **Move idle detection into the agent** and emit _interval_ records (window X focused from t0–t1, active vs idle) instead of point samples — smaller, and directly answers "how long."
2. **Persist via a writer thread / queue** so the network loop never blocks on disk, and consider **SQLite** (already half-scaffolded in the CMake comments) instead of hand-rolled per-day text + directory-scan queries — your `queryRawDataDir` complexity is re-implementing an index that SQLite gives for free, and kidmon-reports being "slow" (the open Todo item) is a symptom of scanning flat files.
3. **One schema, one codec** (D1), validated and bounded at the trust boundary (C1/C4).

## In Progress

*

## Done

* Invalid json causes server to stop
    * Consider catching exception in the services, as an invalid json which causes an exception eventually stops the server
    * It happened when I have run tests while server is running
    * Logs
        * kidmon-server-2024-08-31.log
        * kidmon-server-2024-08-08.log
* Server and agent should have their own config
* Figure out why u8'*' tests are failing on linux
* Add version to the app
* Range matcher loop bug in metricsReview (CmdLine.cpp:22-29): the quote-stripping while tests line.size() (constant) instead of sv.size(); on an all-quotes input sv.front() is UB. Debug-path only, but it's a real bug.
* Config has two sources of truth for dry_run. The member defaults to true (Config.h:90), applyDefaults never sets it, the CLI default is false, and the README says false. It currently works only because cxxopts' default_value makes contains("dry-run") true. Remove the CLI default and the safety behavior silently flips. Pick one authoritative default.
* Hidden benchmark mode in the CLI. CmdLine.cpp:101-106: passing --cfg-file something.txt (any non-.toml extension) silently reinterprets it as a file list, runs metricsReview, and exits. A user who points at the wrong config gets a confusing no-op. That's a debug feature leaking into the production path — it should be an explicit hidden flag.
* Manual recursion in enumPathsRecursive and Node traversals risks stack overflow on pathological depth; iterative + explicit stack is safer for a tool pointed at arbitrary filesystems.
* Code quality / maintainability
    * detect() carries an awkward dual-container reconciliation. DuplicateDetector.cpp:111-200 It builds ordered (a vector copy of dups_) purely for smoother progress, mutates it, then has to rebuild a surviving set to sync deletions back into dups_, and then enumGroups re-derives groups from dups_+grps_ via a visit set. grps_ is already keyed by hash and holds exactly the groups — iterating dups_ (keyed by size) to rediscover them is redundant and fragile. This function would benefit from being decomposed and from a single source of truth for groups.
* Performance — the gap vs. real dedup tools
    * No parallel hashing. Detection is fully single-threaded (DuplicateDetector.cpp:126-177). Hashing is the dominant cost and is embarrassingly parallel across same-size buckets. On a large photo/video corpus this is the difference between minutes and seconds. This is the single biggest performance limitation.
    * No partial/progressive hashing. Same-size files go straight to a full SHA-256 of the entire file. Production dedupers hash the first ~4–64 KB first and only full-hash the survivors. With many same-size-but-different files (extremely common for media), you're reading entire multi-GB files needlessly. SHA-256 is also overkill for a candidate check — a fast non-crypto hash (xxHash/BLAKE3) for screening, with full compare/crypto only on collision, would be much faster.
  
### Kidmon issues

**C1 — Path traversal → arbitrary file write, potentially as root.** In [FileSystemRepository::add](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/repo/FileSystemRepository.cpp#L262) the snapshot is written to `snapshotsDir / entry.windowInfo.image.name`, and `image.name` comes straight off the wire ([DataHandler](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/handler/DataHandler.cpp#L38) → `fromJson`). The server never validates it. A client that sets `image.name = "../../../../etc/cron.d/x"` and arbitrary bytes gets an arbitrary file write with the server's privileges. The username path component is checked against the active user, but the image filename is not. **Sanitize to a basename and reject any name containing separators or `..`.**

**C2 — The auth token is not cryptographically random.** [generateToken](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/common/Utils.cpp#L10) uses C `rand()` seeded with `time(nullptr)`. For a token that gates the data channel this is guessable — the seed is the spawn second, and `rand()` is low-entropy. OpenSSL is already a dependency; use `RAND_bytes`. As-is the "16 chars from a 62-char alphabet" is far weaker than it looks.

**C3 — The token is passed on the command line.** [healthCheck](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/kidmon/src/server/KidmonServer.cpp#L109) launches the agent with `--token <token>`. On every OS, command-line arguments are world-readable from the process table (`ps -ef`, Task Manager, `/proc/<pid>/cmdline`). **Any local user can read the token and impersonate the agent** — connect to the loopback port, pass the token, and inject fabricated entries (or trigger C1). For a local-trust protocol, pass the token via an inherited pipe / stdin / env-scrubbed channel, not argv.

**C4 — Unbounded frame length → trivial local DoS / OOM.** [Unpacker::readSize](vscode-webview://1gb5lhr0m94kasl6evhkc8d1ki7u850arsrvkfgpjgp7a5sr2v4t/core/src/network/data/Unpacker.cpp#L62) does `rem_ = *reinterpret_cast<size_t*>(buffer_.data())` with no upper bound, and `put` appends incoming bytes into an ever-growing buffer. A local client sends a huge length prefix and the server grows memory until it dies. (Also: that `reinterpret_cast` is an unaligned, host-endian read — UB-adjacent, and non-portable, though both ends are local.) Add a sane max-frame cap and reject oversized headers.