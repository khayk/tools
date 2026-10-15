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
| `-H, --hours <n>` | Include the last `n` hours |
| `-d, --days <n>` | Include the last `n` days |
| `-M, --months <n>` | Include the last `n` months |
| `-r, --range <from,to>` | Date range in `YYYYMMDD` format (e.g. `20240101,20241231`) |
| `-p, --process <name>` | Include only entries matching this process (repeatable, see [Process matching](#process-matching)) |
| `-t, --title <text>` | Include only entries matching this window title (repeatable) |
| `--exclude-process <name>` | Exclude entries matching this process (repeatable) |
| `--exclude-title <text>` | Exclude entries matching this window title (repeatable) |
| `-T, --top <n[,m…]>` | Show the top entries per level: processes, paths, titles. The last value applies to deeper levels, e.g. `10,3` shows 10 processes with 3 titles each. `0` hides a level and everything below it, e.g. `10,0` lists processes only (default: 10) |
| `-c, --case-sensitive` | Enable case-sensitive matching (default: case-insensitive) |
| `--source <repo>` | The repository to read from (default: the kidmon reports folder, see [Repositories](#repositories)) |
| `--migrate-to <repo>` | Copy all data from the source into this repository and exit |
| `-h, --help` | Print usage |

### Examples

```bash
# List users that have recorded data
kidmon-reports --list

# What did alice do today?
kidmon-reports --user alice --days 1

# Top 5 processes used in the last 2 hours, with their 3 main window titles
kidmon-reports --user alice --hours 2 --top 5,3

# Activity for a specific date range, filtered to browser windows
kidmon-reports --user alice --range 20240601,20240630 --process firefox

# Time spent in VS Code over the last week, excluding test runs
kidmon-reports --user alice --days 7 --process code --exclude-title "Running Tests"

# Copy all data into a SQLite database
kidmon-reports --migrate-to activity.db

# Query a SQLite database instead of the reports folder
kidmon-reports --source activity.db --user alice --days 7
```

### Process matching

`--process` and `--exclude-process` match the executable name, unless the value contains a path separator (`/` or `\`), in which case they match anywhere in the full path:

```bash
kidmon-reports --user alice --process code                         # executable named *code*
kidmon-reports --user alice --process "Visual Studio Code.app/"    # anywhere in the path
kidmon-reports --user alice --exclude-process /System/             # skip system apps
```

## Output format

Results are grouped hierarchically by process name → process path → window title, each with total duration. For example, `kidmon-reports --user alice --days 1 --top 2,2`:

```
Filtered: 5690 out of 5690
total duration: 6h 25m
    proc_name: code, duration: 4h 12m
        proc_path: /usr/bin/code, duration: 4h 12m
            title: kidmon/README.md, duration: 1h 5m
            title: core/README.md, duration: 45m
    proc_name: firefox, duration: 2h 3m
        proc_path: /usr/bin/firefox, duration: 2h 3m
            title: <empty>, duration: 1h 10m
            title: YouTube - Mozilla Firefox, duration: 40m
```

`Filtered` shows how many entries matched the filters out of all entries in the time range. Windows without a title are shown as `<empty>`.

The depth of the output is currently fixed at three levels: process name → full process path → window title.

## Repositories

A repository is given as `[type:]location`, where type is `fs` (a reports folder) or `sqlite` (a database file). Without a type, an existing folder is `fs` and a `.db`, `.sqlite` or `.sqlite3` file is `sqlite`:

```bash
kidmon-reports --source /mnt/reports --list
kidmon-reports --source sqlite:/backup/activity.data --list
kidmon-reports --source activity.db --migrate-to fs:/backup/reports
```

`--migrate-to` copies every user's data; the query options are ignored. A SQLite target skips entries it already has, so re-running it adds only new data. A folder target must be empty or not exist yet, since writing to a folder only appends and a repeated migration would duplicate every entry.

By default, `kidmon-reports` reads from the same folder that `kidmon` writes to:

| Platform | Default path |
|---|---|
| Linux | `~/.local/share/kidmon/reports/` |
| macOS | `~/Library/Application Support/kidmon/reports/` |
| Windows | `%APPDATA%\kidmon\reports\` |

Pass `--source` to read from another repository.
