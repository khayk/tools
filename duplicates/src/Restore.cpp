#include <duplicates/Restore.h>
#include <core/utils/Crypto.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <format>
#include <optional>
#include <regex>
#include <stdexcept>
#include <system_error>
#include <tuple>

namespace tools::dups {
namespace {

/// Left in a run directory whose restore did not complete, so listings can tell
/// a partly restored run (journal reduced to the pending entries) from a fresh one.
constexpr std::string_view PARTIAL_MARKER = "restore_incomplete";

constexpr std::string_view LATEST = "latest";

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
    const auto marker = runDir / PARTIAL_MARKER;

    if (report.complete())
    {
        fs::remove(journal);
        fs::remove(marker);
    }
    else
    {
        rewriteJournal(journal, report);
        core::file::write(marker, "");
    }

    removeEmptyDirs(runDir);

    // Only an empty run directory is removed: anything unexpected left inside
    // (e.g. a backup no journal entry points to) is kept rather than destroyed.
    std::error_code ec;
    report.runDirRemoved = fs::remove(runDir, ec);
}

/**
 * @brief Sort key and display time of a run, parsed from its directory name
 *        "deleted_YYYYMMDD_HHMMSS[_n]" (see BackupAndDelete).
 */
struct RunName
{
    std::string stamp;  // "YYYYMMDD_HHMMSS", sorts chronologically
    unsigned suffix {}; // same-second runs: 0, 1, 2, ...
    std::string time;
};

std::optional<RunName> parseRunName(const std::string& name)
{
    static const std::regex pattern(
        R"(deleted_((\d{4})(\d{2})(\d{2})_(\d{2})(\d{2})(\d{2}))(?:_(\d{1,9}))?)");

    std::smatch m;
    if (!std::regex_match(name, m, pattern))
    {
        return std::nullopt;
    }

    return RunName {
        .stamp = m[1].str(),
        .suffix = m[8].matched ? static_cast<unsigned>(std::stoul(m[8].str())) : 0U,
        .time = std::format("{}-{}-{} {}:{}:{}",
                            m[2].str(),
                            m[3].str(),
                            m[4].str(),
                            m[5].str(),
                            m[6].str(),
                            m[7].str()),
    };
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

std::vector<RunInfo> listRuns(const fs::path& backupDir)
{
    std::vector<std::pair<RunName, RunInfo>> runs;
    std::error_code ec;

    // A missing backup directory simply means nothing was ever deleted.
    for (const auto& dirEntry : fs::directory_iterator(backupDir, ec))
    {
        auto name = core::file::path2s(dirEntry.path().filename());
        auto parsed = parseRunName(name);
        const auto journal = dirEntry.path() / JOURNAL_FILE_NAME;

        if (!parsed || !dirEntry.is_directory(ec) || !fs::is_regular_file(journal, ec))
        {
            continue;
        }

        RunInfo info {
            .id = std::move(name),
            .dir = dirEntry.path(),
            .time = parsed->time,
            .entries = readJournal(journal).size(),
            .partial = fs::exists(dirEntry.path() / PARTIAL_MARKER, ec),
        };
        runs.emplace_back(std::move(*parsed), std::move(info));
    }

    std::ranges::sort(runs, [](const auto& a, const auto& b) {
        return std::tie(a.first.stamp, a.first.suffix) >
               std::tie(b.first.stamp, b.first.suffix);
    });

    std::vector<RunInfo> result;
    result.reserve(runs.size());
    for (auto& run : runs)
    {
        result.push_back(std::move(run.second));
    }

    return result;
}

RunInfo findRun(const fs::path& backupDir, std::string_view spec)
{
    auto runs = listRuns(backupDir);

    if (runs.empty())
    {
        throw std::runtime_error(
            std::format("No restorable runs found in: {}", backupDir));
    }

    if (spec == LATEST)
    {
        return std::move(runs.front());
    }

    const auto it = std::ranges::find(runs, spec, &RunInfo::id);
    if (it == runs.end())
    {
        throw std::runtime_error(std::format(
            "Run not found: '{}', use --list-runs to see the available runs",
            spec));
    }

    return std::move(*it);
}

} // namespace tools::dups
