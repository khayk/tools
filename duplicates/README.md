# duplicates

Command-line tool that scans directories for duplicate files, identified by SHA-256 hash, and lets you delete them interactively or automatically.

## How it works

1. Scans all specified directories recursively and builds a file list.
2. Groups files with identical SHA-256 hashes.
3. For each duplicate group, decides what to delete based on configured keep/delete path rules.
4. If no automatic rule applies, prompts interactively.

Files outside the configured size range are skipped. Deleted files are moved to a backup cache directory before removal, not permanently erased immediately.

## Configuration

Copy `dups.toml` and edit it, or pass everything on the command line. Both can be combined — command-line flags override the config file.

```toml
# dups.toml

# Directories to scan
scan_directories = [
    "/path/to/photos",
    "/path/to/backup"
]

# Regex patterns for files/directories to skip
exclusion_patterns = [
    "\\.(log|zip|txt)$"
]

# Prefer keeping files from these paths when resolving duplicates
dirs_to_keep_from = [
    "/path/to/photos"
]

# Automatically delete duplicates found entirely within these paths
dirs_to_delete_from = [
    "/path/to/backup"
]

# Size limits (bytes)
min_file_size_bytes = 1024          # skip files smaller than 1 KB
max_file_size_bytes = 10737418240   # skip files larger than 10 GB

# Progress update interval (ms); 0 disables progress output
update_freq_ms = 100

# Output files written to the cache directory
all_files    = "all.txt"        # all scanned paths
dup_files    = "duplicates.txt" # detected duplicate groups
ign_files    = "ignored.txt"    # files marked as ignored across runs

# Preview what would be deleted without actually deleting anything
dry_run = false
```

## Usage

```
duplicates [options]
```

### Options

| Flag | Default | Description |
|---|---|---|
| `--cfg-file <path>` | `dups.toml` | Config file to load |
| `--scan-dir <path>` | — | Directory to scan (repeatable) |
| `--exclude <regex>` | — | Exclusion pattern (repeatable) |
| `--keep-path <path>` | — | Prefer keeping files from this path (repeatable) |
| `--delete-path <path>` | — | Auto-delete duplicates from this path (repeatable) |
| `--min-size <bytes>` | `1024` | Ignore files smaller than this |
| `--max-size <bytes>` | `10737418240` | Ignore files larger than this |
| `--dry-run` | `false` | Preview deletions without performing them |
| `--update-freq <ms>` | `100` | Progress update frequency |
| `-h, --help` | | Print usage |

### Examples

```bash
# Scan two directories using the default config file
duplicates --scan-dir ~/Photos --scan-dir ~/Backup

# Dry run with an explicit config
duplicates --cfg-file my-config.toml --dry-run

# Everything via command line, prefer keeping files from ~/Photos
duplicates \
  --scan-dir ~/Photos \
  --scan-dir ~/Backup \
  --keep-path ~/Photos \
  --delete-path ~/Backup \
  --exclude "\\.log$"
```

## Deletion behaviour

- **`dirs_to_keep_from`** / `--keep-path`: if a duplicate group has exactly one file matching a keep path, the rest are deleted automatically.
- **`dirs_to_delete_from`** / `--delete-path`: if all files in a duplicate group are inside a delete path, deletion is confirmed interactively.
- If neither rule matches, the tool prompts for each group.
- `--dry-run` logs what *would* be deleted without touching anything.

## Output files

Written to the platform cache directory (printed at startup):

| File | Contents |
|---|---|
| `all.txt` | Every file path that was scanned |
| `duplicates.txt` | Duplicate groups (one group per block) |
| `ignored.txt` | Paths the user chose to ignore; persisted across runs |
