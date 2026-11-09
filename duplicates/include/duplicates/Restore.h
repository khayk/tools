#pragma once

#include <duplicates/Journal.h>
#include <duplicates/Progress.h>

#include <filesystem>
#include <string>
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
 * @param runDir Run directory holding the journal and the backups.
 * @param opts Restore options.
 *
 * @throw std::runtime_error if @p runDir is not a run directory with a journal.
 */
RestoreReport restoreRun(const fs::path& runDir, const RestoreOptions& opts = {});

} // namespace tools::dups
