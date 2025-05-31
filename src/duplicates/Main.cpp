#include <duplicates/DuplicateDetector.h>
#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DuplicateOperation.h>
#include <duplicates/Config.h>

#include <core/utils/Log.h>
#include <core/utils/Tracer.h>

#include <filesystem>
#include <fmt/format.h>

#include <system_error>

using tools::dups::DuplicateDetector;
using tools::dups::Config;
using tools::dups::Progress;
using tools::dups::loadConfig;
using tools::dups::logConfig;

namespace {

void printUsage()
{
    puts(R"(
Usage:
    @ todo: revise this
    duplicates <cfg_file> - Scan directories and detect duplicates based on the
                            configuration file.
    duplicates <dir>   - Search duplicate items in the given directory.
)");
}

} // namespace

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printUsage();
        return 2;
    }

    std::optional<ScopedTrace> trace;

    try
    {
        const fs::path cfgFile(argv[1]);
        const Config cfg = loadConfig(cfgFile);
        utl::configureLogger(cfg.logDir, cfg.logFilename);
        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
        logConfig(cfgFile, cfg);

        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);

        scanDirectories(cfg, detector, progress);
        dumpContent(cfg.allFilesPath, detector);

        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg.dupFilesPath, detector);
        deleteDuplicates(cfg, detector);
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
