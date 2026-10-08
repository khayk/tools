# kidmon-reports review

_Review date: 2026-10-07 (commit `df75ea6`). Covers all of `kidmon-reports/` (about 1.3k lines) and the repository code it calls._

## Overall

The structure is good. Conditions (`ICondition` with And/Or/Not) let you build filter expressions, transforms are kept apart from conditions, and the aggregation is a typed template tree. It's a solid base, but the tool looks half-finished: `Main.cpp` mixes command-line parsing, query building, display and dead code, there are no tests (`test/CMakeLists.txt` is empty), and several options don't do what the README says.

## Bugs (fix first)

1. [x] **Matching is case-sensitive by default, but the README says the opposite.** `ReportsConfig::caseSensitive {true}` stays `true` when `-c` isn't passed, so `-c` does nothing and case-insensitive matching is never used.
2. [x] **Exclude values are never lowercased.** `applyCaseTransform` lowercases `titles` and `processes`, but not `excludeTitles` or `excludeProcesses`. With case-insensitive matching on, `--exclude-title "Running Tests"` would never match.
3. [x] **"Filtered: 0 out of N" every time.** `entries_` is never filled. `add()` also builds a lowercase `procname` and then throws it away.
4. [x] **Transforms change the output.** The lowercasing transforms rewrite the entry in place before it is grouped. The report then shows lowercased paths and titles, but only when certain filters are given. Matching should lowercase a copy and leave the displayed data alone.
5. [x] **`--range` excludes its end date.** `20240601,20240630` ends at midnight at the start of the 30th, so the 30th is dropped. Users expect the end date to be included, so add a day. Invalid dates like `20241345` are also accepted without any error.
6. [x] **No time option means an empty report.** If none of `-m`, `-h`, `-d`, `-M` or `-r` is given, `from == to` and nothing comes back, with no warning. Default to something sensible like "today" or show an error.
7. [x] **No user, or an unknown user, gives a raw `filesystem_error`.** `getUserDir("")` and missing folders throw from `directory_iterator`. `validateArguments` is a stub with a TODO; this is where it should be caught.
8. [x] **Process matching is a substring search over the full path.** `--process code` matches any path that contains "code", e.g. `/Users/x/code/...`. Match on the file name by default. _Done: matches the executable name, or the full path when the value contains `/` or `\`._
9. [ ] **`--top` applies at every level, not just once.** The default of 10 can mean 10 × 10 × 10 rows. The README describes it as "top n results".
10. [ ] **`--fields` is parsed but never used.** Also, the README's output example doesn't match the real output (`proc_name: code, duration: …` plus `total` lines).

Also fixed along the way:

- [x] **`--top` default was never applied.** cxxopts' `contains()` ignores default values, so `topN` stayed 0 without `-T`.
- [x] **Stray arguments were silently ignored.** E.g. `-title foo` parses as `-t itle` and drops `foo`; leftover arguments are now an error.

## Code I'd change

- [ ] **Split `Main.cpp`.** Separate files for command-line parsing, building the query (filter, condition, transform), running it, and rendering. Rendering should be behind an interface so other output formats can be added. _Partly done: options, query building and `QueryVisualizer` are now separate files in a `kidmon-reports` library. Running the query and rendering behind an interface remain._
- [ ] **Make `QueryVisualizer` only render.** Right now it also counts progress, holds the aggregation tree, and has an unused `buf_`. The progress counter belongs in a decorator or a separate reporter, which matches how stats are kept out of core classes elsewhere in this repo. _Partly done: `buf_` removed. It still counts progress and owns the aggregation tree._
- [ ] **Simplify `combineConditions`.** It builds a deep left-leaning recursive tree. `AnyOf` / `AllOf` conditions holding a `std::vector<ConditionPtr>` would be simpler and print more readably.
- [ ] **Remove the `Data` inheritance.** `Aggregate` derives from `Data`, and `Data` derives from `IAggregate`. Composition (`Data totals_;`) would be clearer.
- [ ] **Shorten `orderedVec`.** `std::ranges::partial_sort` plus `resize` replaces the full sort and the pop-back loop.
- [ ] **Remove dead code.** That's the `Splitter` block, the commented-out `write` calls, the `IsStringCondition` that nothing uses, and the commented-out migration snippet at the end of `Main.cpp`. If the migration is useful, make it a `migrate` subcommand instead of comments.
- [ ] **Fix the short flags.** `-h` for hours and `-e` for help goes against every CLI convention. Use `-h/--help`, plus `--hours` alone or something like `--since 2h`.
- [ ] **Separate logs from the report.** `spdlog::info` goes to the console, so "Users:" and "Query condition:" are mixed into the report, and the `\r` progress line is never cleared. Send logs and progress to stderr and the report to stdout, so the output can be piped.
- [x] **Fix two inconsistencies.** `ProcessPathToLowerTransform` uses `lowerInplace` on a `wstring` while titles use `utf8LowerInplace`. `CMakeLists.txt` uses a `GLOB` that leaves out `src/aggregate/*.h`. _Done: `CMakeLists.txt` lists sources explicitly, and the transforms are gone, so all case-insensitive matching uses `utf8LowerInplace`._
- [ ] **Add tests.** Conditions, date parsing, and aggregation ordering and top-N are all pure logic and easy to test. Bugs 1, 2 and 5 would have been caught. _Partly done: `kidmon-reports-test` covers options, include/exclude conditions, date ranges, user validation and the filtered count. Aggregation ordering and top-N remain._

## What I'd do differently

- [ ] **Use SQLite for reports.** The server already writes to `SqliteRepository`. Grouping, filtering and time ranges become SQL queries (`GROUP BY`, `LIKE`/`GLOB` with `COLLATE NOCASE`), which is much faster for months of data than scanning every raw file. The condition and aggregate classes could become a small query builder.
- [ ] **Let the user choose the grouping.** Instead of the fixed name → path → title hierarchy, add something like `--group-by app,title` or `--group-by day,app`. The `FieldBuilder` template already points that way; making it chosen at runtime (a vector of key extractors) would turn `--fields` into a real feature.
- [ ] **Use regex or glob patterns** for titles and processes instead of plain substrings.

## Features that would make it more useful

For a parental-monitoring tool, the useful questions are "how much, when, and on what":

1. [ ] **Daily and weekly breakdowns.** A per-day table or a simple text histogram of screen time, plus a "time of day" view (e.g. activity after 22:00).
2. [ ] **Categories.** A small config mapping apps or title patterns to categories (Games, Browsing, Education, YouTube…), then reporting by category. This is probably the most valuable addition.
3. [ ] **Website and video detection from browser titles.** Pull out the site or video name ("… – YouTube"), since most of the time is spent inside the browser.
4. [ ] **Sessions and idle time.** Merge consecutive entries into sessions, show the longest continuous session, and the first and last activity of each day. It also needs to be clear whether idle time is counted in `duration`.
5. [ ] **Machine-readable output.** `--format table|json|csv` so results can go into spreadsheets or a dashboard. A simple HTML report with charts would also suit a parent better than a terminal tree.
6. [ ] **Comparisons.** This week vs last week, and trends over time.
7. [ ] **Limits and alerts.** "Games > 2h/day" checks that can be run from cron and send a notification. This builds on categories.
8. [ ] **Percentages and totals.** Each row's share of the total, plus an overall total and the period covered in the header.
9. [ ] **Screenshots.** `WindowInfo` carries an `Image`; a "show captures for this title or time" option would let a parent check context.
10. [ ] **Comparing users** in one run, e.g. `--user alice,bob` or `--all-users`.

## Suggested order

Fix bugs 1–3 and 5–7 and add tests for them. Then split `Main.cpp`, add `--format json` and `--group-by`, and after that categories and per-day reports.
