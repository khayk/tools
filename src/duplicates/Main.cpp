#include <duplicates/DuplicateDetector.h>
#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DuplicateOperation.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Config.h>
#include <duplicates/Node.h>

#include <core/utils/Log.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Tracer.h>
#include <core/utils/Sys.h>
#include <core/utils/Str.h>
#include <filesystem>
#include <fmt/format.h>

#include <memory>
#include <system_error>

using tools::utl::configureLogger;

namespace tools::dups {
namespace {

void printUsage()
{
    puts(R"(
Usage:
    @ todo: revise this
    duplicates <cfg_file> - Scan directories and detect duplicates based on the
                            configuration file.
)");
}

std::unique_ptr<IDeletionStrategy> getDeletionStrategy(const Config& cfg)
{
    if (cfg.dryRun)
    {
        return std::make_unique<DryRunDelete>();
    }

    return std::make_unique<BackupAndDelete>(cfg.cacheDir);
}

void reviewMetrics(const fs::path& file)
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
} // namespace tools::dups

int main(int argc, const char* argv[])
{
    using namespace tools::dups;

    if (argc < 2)
    {
        printUsage();
        return 2;
    }

    std::optional<ScopedTrace> trace;

    try
    {
        const fs::path cfgFile(argv[1]);

        if (cfgFile.extension() != ".toml")
        {
            spdlog::info("Measuring resource usage: '{}'", cfgFile);
            reviewMetrics(cfgFile);
            return 0;
        }

        const Config cfg = loadConfig(cfgFile);
        configureLogger(cfg.logDir, cfg.logFilename);
        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
        logConfig(cfgFile, cfg);

        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);
        IgnoredFiles ignored(cfg.ignFilesPath);

        scanDirectories(cfg, detector, progress);
        outputFiles(cfg.allFilesPath, detector);

        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg.dupFilesPath, detector);

        auto strategy = getDeletionStrategy(cfg);

        deleteDuplicates(*strategy,
                         detector,
                         cfg.safeToDeleteDirs,
                         ignored,
                         std::cout,
                         std::cin);
    }
    catch (const std::system_error& se)
    {
        spdlog::error("std::system_error: {}", se.what());
        return 1;
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
        return 1;
    }

    return 0;
}
