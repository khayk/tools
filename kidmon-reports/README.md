# kidmon-reports

Query tool for activity data collected by [kidmon](../kidmon/README.md). Filters entries by user, time range, process name, and window title, then displays a ranked summary of time spent.

## Usage

```
kidmon-reports [options]
```

### Options

| Flag | Description |
|---|---|
| `-l, --list` | List all users that have recorded data |
| `-u, --user <name>` | User to query |
| `-m, --minutes <n>` | Include the last `n` minutes |
| `-h, --hours <n>` | Include the last `n` hours |
| `-d, --days <n>` | Include the last `n` days |
| `-M, --months <n>` | Include the last `n` months |
| `-r, --range <from,to>` | Date range in `YYYYMMDD` format (e.g. `20240101,20241231`) |
| `-p, --process <name>` | Include only entries matching this process name (repeatable) |
| `-t, --title <text>` | Include only entries matching this window title (repeatable) |
| `--exclude-process <name>` | Exclude entries matching this process name (repeatable) |
| `--exclude-title <text>` | Exclude entries matching this window title (repeatable) |
| `-T, --top <n>` | Show top `n` results (default: 10) |
| `-c, --case-sensitive` | Enable case-sensitive matching (default: case-insensitive) |
| `--reports-dir <path>` | Path to the kidmon reports directory (auto-detected if omitted) |
| `-e, --help` | Print usage |

### Examples

```bash
# List users that have recorded data
kidmon-reports --list

# What did alice do today?
kidmon-reports --user alice --days 1

# Top 5 processes used in the last 2 hours
kidmon-reports --user alice --hours 2 --top 5

# Activity for a specific date range, filtered to browser windows
kidmon-reports --user alice --range 20240601,20240630 --process firefox

# Time spent in VS Code over the last week, excluding test runs
kidmon-reports --user alice --days 7 --process code --exclude-title "Running Tests"

# Query a specific reports directory
kidmon-reports --user alice --days 7 --reports-dir /mnt/reports
```

## Output format

Results are grouped hierarchically by process name → process path → window title, each with total duration:

```
code                        duration: 4h 12m
    /usr/bin/code           duration: 4h 12m
        kidmon/README.md    duration: 1h 05m
        core/README.md      duration: 45m
firefox                     duration: 2h 03m
    ...
```

The depth of the output is currently fixed at three levels: process name → full process path → window title.

## Data directory

By default, `kidmon-reports` reads from the same directory that `kidmon` writes to:

| Platform | Default path |
|---|---|
| Linux | `~/.local/share/kidmon/reports/` |
| macOS | `~/Library/Application Support/kidmon/reports/` |
| Windows | `%APPDATA%\kidmon\reports\` |

Pass `--reports-dir` to override this.
