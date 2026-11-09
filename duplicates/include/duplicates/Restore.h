#pragma once

#include <duplicates/Journal.h>
#include <duplicates/Progress.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace tools::dups {

/**
 * @brief An entry that could not be restored because of an I/O error.
 */
struct RestoreFailure
{
    JournalEntry entry;
    std::string error;
};

/**
 * @brief Outcome of restoring one run, with every journal entry in exactly one
 *        bucket. In a dry run the buckets describe what would happen.
 */
struct RestoreReport
{
    /// Backup moved back to its original location.
    std::vector<JournalEntry> restored;

    /// Original already exists with identical content; the backup was dropped.
    std::vector<JournalEntry> identical;

    /// Original already exists with different content; both were left untouched.
    std::vector<JournalEntry> conflicts;

    /// Backup not found: restored earlier, or the app stopped between journaling
    /// the deletion and moving the file.
    std::vector<JournalEntry> missing;

    /// An I/O error occurred; the backup is kept so the restore can be retried.
    std::vector<RestoreFailure> failed;

    /// The run directory was fully restored and removed.
    bool runDirRemoved {false};

    /// Set when the files were processed but tidying up the run directory
    /// afterwards failed (e.g. disk full). Repeating the restore is safe.
    std::optional<std::string> cleanupError;

    size_t total() const noexcept;

    /// True when nothing is left to restore (no conflicts and no failures).
    bool complete() const noexcept;
};

struct RestoreOptions
{
    /// Classify the entries and log what would happen, without touching any file.
    bool dryRun {false};

    /// Optional progress reporter, updated as entries are processed.
    Progress* progress {nullptr};
};

/**
 * @brief Restore the files deleted by one run of the application, by replaying the
 *        run's journal (see BackupAndDelete).
 *
 * Existing files are never overwritten. When the restore completes, the run
 * directory is removed; otherwise the journal is rewritten to hold only the
 * entries that are still pending (conflicts and failures), so the restore can be
 * repeated after resolving them.
 *
 * Errors while tidying up the run directory do not discard the report: they are
 * returned in RestoreReport::cleanupError.
 *
 * Do not restore a run that another session of the application is still
 * deleting into.
 *
 * @param runDir Run directory holding the journal and the backups.
 * @param opts Restore options.
 *
 * @throw std::runtime_error if @p runDir is not a run directory with a journal.
 * @throw std::exception if the journal cannot be read.
 */
RestoreReport restoreRun(const fs::path& runDir, const RestoreOptions& opts = {});

/**
 * @brief A run whose deletions can still be restored.
 */
struct RunInfo
{
    /// Run directory name, e.g. "deleted_20261108_141500"; identifies the run.
    std::string id;

    fs::path dir;

    /// When the run started, as "YYYY-MM-DD HH:MM:SS".
    std::string time;

    /// Number of entries in the journal that are still pending, or std::nullopt
    /// if the journal could not be read.
    std::optional<size_t> entries;

    /// An earlier restore of this run left conflicts or failures behind.
    bool partial {false};
};

/**
 * @brief Find the restorable runs in the backup directory, newest first.
 *        Only directories named like a run and holding a journal are listed.
 *        A run whose journal cannot be read is still listed, without an entry
 *        count, so it does not hide the other runs.
 *
 * @param backupDir Directory the runs live in (the BackupAndDelete backup root).
 *
 * @return The runs; empty if @p backupDir does not exist.
 * @throw std::filesystem::filesystem_error if @p backupDir exists but cannot be
 *        listed (e.g. no permission, or not a directory).
 */
std::vector<RunInfo> listRuns(const fs::path& backupDir);

/**
 * @brief Select a run by its id, or the newest run with "latest".
 *
 * @throw std::runtime_error if there is no such run.
 */
RunInfo findRun(const fs::path& backupDir, std::string_view spec);

} // namespace tools::dups
