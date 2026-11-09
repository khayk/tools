#include <duplicates/Restore.h>
#include <core/utils/Crypto.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <format>
#include <stdexcept>
#include <system_error>

namespace tools::dups {
namespace {

enum class Action : uint8_t
{
    Restore,
    DropIdentical,
    Conflict,
    Missing
};

bool pathTaken(const fs::path& path)
{
    // Look at the link itself: a dangling symlink still occupies the path and
    // must not be overwritten.
    std::error_code ec;
    return fs::exists(fs::symlink_status(path, ec));
}

bool sameContent(const fs::path& a, const fs::path& b)
{
    if (!fs::is_regular_file(a) || !fs::is_regular_file(b) ||
        fs::file_size(a) != fs::file_size(b))
    {
        return false;
    }

    return core::crypto::fileSha256(a) == core::crypto::fileSha256(b);
}

Action classify(const JournalEntry& entry)
{
    if (!pathTaken(entry.backup))
    {
        return Action::Missing;
    }

    if (!pathTaken(entry.original))
    {
        return Action::Restore;
    }

    return sameContent(entry.original, entry.backup) ? Action::DropIdentical
                                                     : Action::Conflict;
}

void moveBack(const JournalEntry& entry)
{
    fs::create_directories(entry.original.parent_path());

    // The original was checked to be absent just before. On POSIX rename would
    // still replace a file created in that short window; this tool does not
    // guard against concurrent writers to the same path.
    std::error_code ec;
    fs::rename(entry.backup, entry.original, ec);

    if (ec)
    {
        // Typically a cross-device move. copy_options::none refuses to overwrite,
        // so a file that appeared at the original path meanwhile is preserved.
        fs::copy_file(entry.backup, entry.original, fs::copy_options::none);
        fs::remove(entry.backup);
    }
}

void process(const JournalEntry& entry, bool dryRun, RestoreReport& report)
{
    try
    {
        switch (classify(entry))
        {
            case Action::Restore:
                if (!dryRun)
                {
                    moveBack(entry);
                }
                spdlog::info("{}: {}",
                             dryRun ? "Would restore" : "Restored",
                             entry.original);
                report.restored.push_back(entry);
                break;

            case Action::DropIdentical:
                if (!dryRun)
                {
                    fs::remove(entry.backup);
                }
                spdlog::info("Already present with identical content: {}",
                             entry.original);
                report.identical.push_back(entry);
                break;

            case Action::Conflict:
                spdlog::warn("Conflict, a different file exists at: {}, backup kept "
                             "at: {}",
                             entry.original,
                             entry.backup);
                report.conflicts.push_back(entry);
                break;

            case Action::Missing:
                spdlog::warn("Backup not found: {}, skipping: {}",
                             entry.backup,
                             entry.original);
                report.missing.push_back(entry);
                break;
        }
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Failed to restore: {}, error: {}", entry.original, ex.what());
        report.failed.push_back({entry, ex.what()});
    }
}

void rewriteJournal(const fs::path& journal, const RestoreReport& report)
{
    // Write the pending entries next to the journal and swap it in, so the
    // journal is never left half-written.
    const auto tmp = fs::path(journal).concat(".tmp");
    fs::remove(tmp);
    {
        JournalWriter writer(tmp);
        for (const auto& entry : report.conflicts)
        {
            writer.append(entry);
        }
        for (const auto& failure : report.failed)
        {
            writer.append(failure.entry);
        }
    }
    fs::rename(tmp, journal);
}

void removeEmptyDirs(const fs::path& runDir)
{
    std::error_code ec;
    for (const auto& dirEntry : fs::directory_iterator(runDir, ec))
    {
        if (dirEntry.is_directory(ec))
        {
            fs::remove(dirEntry.path(), ec); // fails harmlessly when not empty
        }
    }
}

void cleanUp(const fs::path& runDir, const fs::path& journal, RestoreReport& report)
{
    if (report.complete())
    {
        fs::remove(journal);
    }
    else
    {
        rewriteJournal(journal, report);
    }

    removeEmptyDirs(runDir);

    // Only an empty run directory is removed: anything unexpected left inside
    // (e.g. a backup no journal entry points to) is kept rather than destroyed.
    std::error_code ec;
    report.runDirRemoved = fs::remove(runDir, ec);
}

} // namespace

size_t RestoreReport::total() const noexcept
{
    return restored.size() + identical.size() + conflicts.size() + missing.size() +
           failed.size();
}

bool RestoreReport::complete() const noexcept
{
    return conflicts.empty() && failed.empty();
}

RestoreReport restoreRun(const fs::path& runDir, const RestoreOptions& opts)
{
    const auto journal = runDir / JOURNAL_FILE_NAME;

    if (!fs::is_regular_file(journal))
    {
        throw std::runtime_error(
            std::format("Not a run directory, journal not found: {}", journal));
    }

    const auto entries = readJournal(journal);
    spdlog::info("{} {} entries from: {}",
                 opts.dryRun ? "Checking" : "Restoring",
                 entries.size(),
                 runDir);

    RestoreReport report;
    size_t done = 0;

    for (const auto& entry : entries)
    {
        process(entry, opts.dryRun, report);
        ++done;

        if (opts.progress)
        {
            opts.progress->update([&](std::ostream& os) {
                os << "Restoring: " << done << " out of " << entries.size();
            });
        }
    }

    if (!opts.dryRun)
    {
        cleanUp(runDir, journal, report);
    }

    spdlog::info("Restored: {}, identical: {}, conflicts: {}, missing: {}, "
                 "failed: {}",
                 report.restored.size(),
                 report.identical.size(),
                 report.conflicts.size(),
                 report.missing.size(),
                 report.failed.size());

    return report;
}

} // namespace tools::dups
