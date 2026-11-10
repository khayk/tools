#include <duplicates/CmdLine.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Restore.h>
#include <core/utils/StopWatch.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>

#include <format>

namespace tools::dups {
namespace {

void metricsReview(const fs::path& file)
{
    DuplicateDetector detector;
    StopWatch sw;

    core::file::readLines(file, [&detector](const std::string& line) {
        std::string_view sv = line;

        // Strip surrounding double quotes (paths may be dumped quoted). Guard on
        // sv.size() - not line.size() - so the view never empties under
        // front()/back().
        while (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"')
        {
            sv.remove_prefix(1);
            sv.remove_suffix(1);
        }

        detector.addFile(sv);
        return true;
    });

    spdlog::info("Memory: {}",
                 core::str::humanizeBytes(core::sys::currentProcessMemoryUsage()));
    spdlog::info("Files: {}", detector.numFiles());
    spdlog::info("Nodes: {}", detector.root()->nodesCount());
    spdlog::info("Elapsed: {} ms", sw.elapsedMs());
    spdlog::info("sizeof(path): {}", sizeof(fs::path));
    spdlog::info("sizeof(node): {}", sizeof(Node));
    spdlog::info("sizeof(string): {}", sizeof(std::string));
}

std::string_view runStatus(const RunInfo& run)
{
    if (!run.entries)
    {
        return "unreadable journal";
    }

    return run.partial ? "partly restored" : "pending";
}

void printRuns(const std::vector<RunInfo>& runs,
               const fs::path& backupDir,
               std::ostream& out)
{
    if (runs.empty())
    {
        out << std::format("No restorable runs in: {}\n", backupDir);
        return;
    }

    constexpr auto row = "{:<28}  {:<19}  {:>8}  {}\n";
    out << std::format(row, "Run", "Time", "Entries", "Status");
    for (const auto& run : runs)
    {
        out << std::format(row,
                           run.id,
                           run.time,
                           run.entries ? std::to_string(*run.entries) : "?",
                           runStatus(run));
    }
}

bool succeeded(const RestoreReport& report)
{
    return report.complete() && !report.cleanupError;
}

/// One pending entry of the summary, shared by every list so they read alike.
void printEntry(const JournalEntry& e, std::ostream& out)
{
    out << std::format("  {}\n    backup: {}\n", e.original, e.backup);
}

void printReport(const RunInfo& run,
                 const RestoreReport& report,
                 bool dryRun,
                 std::ostream& out)
{
    out << std::format("{}: {}, identical: {}, conflicts: {}, missing: {}, "
                       "failed: {}\n",
                       dryRun ? "Would restore" : "Restored",
                       report.restored.size(),
                       report.identical.size(),
                       report.conflicts.size(),
                       report.missing.size(),
                       report.failed.size());

    if (!report.conflicts.empty())
    {
        out << "Conflicts, a different file exists at the original path:\n";
        for (const auto& e : report.conflicts)
        {
            printEntry(e, out);
        }
    }

    if (!report.failed.empty())
    {
        out << "Failed:\n";
        for (const auto& f : report.failed)
        {
            printEntry(f.entry, out);
            out << std::format("    error: {}\n", f.error);
        }
    }

    if (report.cleanupError)
    {
        out << std::format("Files were processed, but tidying up the run failed: "
                           "{}\n",
                           *report.cleanupError);
    }

    if (!succeeded(report) && !dryRun)
    {
        out << std::format("Backups were kept. Resolve the above and run "
                           "'--restore {}' again.\n",
                           run.id);
    }
}

} // namespace

void defineOptions(cxxopts::Options& opts)
{
    // clang-format off
    opts.add_options()
        ("cfg-file", "Configuration as a file",
            cxxopts::value<std::string>()->default_value("dups.toml"))

        ("scan-dir", "Directory to scan (repeatable)",
            cxxopts::value<std::vector<std::string>>())

        ("exclude", "Exclude regex pattern (repeatable)",
            cxxopts::value<std::vector<std::string>>())

        ("keep-path", "Path to keep from (repeatable)",
            cxxopts::value<std::vector<std::string>>())

        ("delete-path", "Path to delete from (repeatable)",
            cxxopts::value<std::vector<std::string>>())

        ("min-size", "Ignore files smaller than this (bytes)",
            cxxopts::value<uint64_t>()->default_value("1024"))

        ("max-size", "Ignore files larger than this (bytes)",
            cxxopts::value<uint64_t>()->default_value("10737418240"))

        ("update-freq", "Progress update frequency (ms, 0 disables)",
            cxxopts::value<uint64_t>()->default_value("100"))

        ("all-files", "File to dump all scanned files",
            cxxopts::value<std::string>()->default_value("all.txt"))

        ("dup-files", "File to dump duplicate files",
            cxxopts::value<std::string>()->default_value("duplicates.txt"))

        ("ign-files", "File to store ignored files",
            cxxopts::value<std::string>()->default_value("ignored.txt"))

        ("dry-run", "Emulate deletion (or restore) instead of performing it",
            cxxopts::value<bool>())

        ("list-runs", "List the runs whose deletions can be restored")

        ("restore", "Restore the files deleted by a run: 'latest' or a run id",
            cxxopts::value<std::string>())

        ("v,version", "Print version information")

        ("h,help", "Print usage");

    // Hidden debug-only options, kept out of --help (see Main.cpp).
    opts.add_options("hidden")
        ("metrics-file", "Load the listed paths and report resource usage",
            cxxopts::value<std::string>());
    // clang-format on
}


bool runMetricsReview(const cxxopts::ParseResult& opts)
{
    if (!opts.contains("metrics-file"))
    {
        return false;
    }

    const fs::path file = opts["metrics-file"].as<std::string>();
    spdlog::info("Measuring resource usage: '{}'", file);
    metricsReview(file);
    return true;
}


std::optional<int> runRestoreCommands(const cxxopts::ParseResult& opts,
                                      const Config& cfg,
                                      std::ostream& out)
{
    const bool list = opts.contains("list-runs");
    const bool restore = opts.contains("restore");

    if (!list && !restore)
    {
        return std::nullopt;
    }

    // BackupAndDelete keeps its runs directly under the cache directory.
    const auto& backupDir = cfg.cacheDir();

    if (list)
    {
        printRuns(listRuns(backupDir), backupDir, out);
    }

    if (!restore)
    {
        return 0;
    }

    const auto run = findRun(backupDir, opts["restore"].as<std::string>());
    out << std::format("{} run {} ({})\n",
                       cfg.dryRun() ? "Checking" : "Restoring",
                       run.id,
                       run.time);

    Progress progress(&out, cfg.updateFrequency());
    const auto report =
        restoreRun(run.dir, {.dryRun = cfg.dryRun(), .progress = &progress});
    printReport(run, report, cfg.dryRun(), out);

    // A dry run leaves nothing behind, whatever it found.
    return cfg.dryRun() || succeeded(report) ? 0 : 2;
}


void populateConfig(const cxxopts::ParseResult& opts, Config& cfg)
{
    applyDefaults(cfg);

    fs::path cfgFile;
    if (opts.contains("cfg-file"))
    {
        cfgFile = opts["cfg-file"].as<std::string>();
    }

    if (!cfgFile.empty())
    {
        applyOverrides(cfgFile, cfg);
    }

    if (opts.contains("dry-run"))
    {
        cfg.setDryRun(opts["dry-run"].as<bool>());
    }

    if (opts.contains("scan-dir"))
    {
        for (const auto& scanDir : opts["scan-dir"].as<std::vector<std::string>>())
        {
            cfg.addScanDir(scanDir);
        }
    }

    if (opts.contains("exclude"))
    {
        for (const auto& excludeDir : opts["exclude"].as<std::vector<std::string>>())
        {
            cfg.addExclusionPattern(excludeDir);
        }
    }

    if (opts.contains("keep-path"))
    {
        for (const auto& keepPath : opts["keep-path"].as<std::vector<std::string>>())
        {
            cfg.addDirToKeepFrom(keepPath);
        }
    }

    if (opts.contains("delete-path"))
    {
        for (const auto& deletePath :
             opts["delete-path"].as<std::vector<std::string>>())
        {
            cfg.addDirToDeleteFrom(deletePath);
        }
    }

    if (opts.contains("min-size"))
    {
        cfg.setMinFileSizeBytes(opts["min-size"].as<uint64_t>());
    }

    if (opts.contains("max-size"))
    {
        cfg.setMaxFileSizeBytes(opts["max-size"].as<uint64_t>());
    }

    if (opts.contains("update-freq"))
    {
        cfg.setUpdateFrequency(
            std::chrono::milliseconds(opts["update-freq"].as<uint64_t>()));
    }

    if (opts.contains("all-files"))
    {
        cfg.setAllFilesPath(opts["all-files"].as<std::string>());
    }

    if (opts.contains("dup-files"))
    {
        cfg.setDupFilesPath(opts["dup-files"].as<std::string>());
    }

    if (opts.contains("ign-files"))
    {
        cfg.setIgnFilesPath(opts["ign-files"].as<std::string>());
    }
}

} // namespace tools::dups
