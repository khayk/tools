#include <duplicates/CmdLine.h>
#include <duplicates/DuplicateDetector.h>
#include <core/utils/StopWatch.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>

namespace tools::dups {
namespace {

void metricsReview(const fs::path& file)
{
    DuplicateDetector detector;
    StopWatch sw;

    file::readLines(file, [&detector](const std::string& line) {
        std::string_view sv = line;

        while (line.size() >= 2 && sv.front() == '"')
        {
            sv.remove_prefix(1);
        }
        while (line.size() >= 2 && sv.back() == '"')
        {
            sv.remove_suffix(1);
        }

        detector.addFile(sv);
        return true;
    });

    spdlog::info("Memory: {}", str::humanizeBytes(sys::currentProcessMemoryUsage()));
    spdlog::info("Files: {}", detector.numFiles());
    spdlog::info("Nodes: {}", detector.root()->nodesCount());
    spdlog::info("Elapsed: {} ms", sw.elapsedMs());
    spdlog::info("sizeof(path): {}", sizeof(fs::path));
    spdlog::info("sizeof(node): {}", sizeof(Node));
    spdlog::info("sizeof(string): {}", sizeof(std::string));
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

        ("dry-run", "Emulate deletion instead of performing it",
            cxxopts::value<bool>()->default_value("false"))

        ("h,help", "Print usage");
    // clang-format on
}


void populateConfig(const cxxopts::ParseResult& opts, Config& cfg)
{
    applyDefaults(cfg);

    fs::path cfgFile;
    if (opts.contains("cfg-file"))
    {
        cfgFile = opts["cfg-file"].as<std::string>();
        applyOverrides(cfgFile, cfg);
    }

    if (!cfgFile.empty() && cfgFile.extension() != ".toml")
    {
        spdlog::info("Measuring resource usage: '{}'", cfgFile);
        metricsReview(cfgFile);
        return;
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
