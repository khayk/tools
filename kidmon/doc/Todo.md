# Todo

* Troubleshoot why the kidmon-reports workings so slow (a small amount of data is processed during ~300ms)

## Open Issues

*

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