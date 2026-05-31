#include <duplicates/DuplicateDetector.h>
#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DuplicateOperation.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Config.h>
#include <duplicates/Node.h>
#include <duplicates/CmdLine.h>
#include <duplicates/Menu.h>

#include <BuildInfo.h>

#include <core/utils/Log.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Tracer.h>
#include <core/utils/Sys.h>
#include <core/utils/Str.h>
#include <core/utils/Dirs.h>

#include <format>
#include <cxxopts.hpp>

#include <memory>
#include <system_error>
#include <iostream>

using core::utl::configureLogger;

namespace tools::dups {
namespace {

std::unique_ptr<IDeletionStrategy> createDeletionStrategy(const Config& cfg)
{
    if (cfg.dryRun())
    {
        return std::make_unique<DryRunDelete>();
    }

    return std::make_unique<BackupAndDelete>(cfg.cacheDir());
}


} // namespace
} // namespace tools::dups

int main(int argc, const char* argv[])
{
    using namespace tools::dups;
    std::optional<ScopedTrace> trace;

    try
    {
        cxxopts::Options opts("duplicates", "Duplicate file detection tool");
        defineOptions(opts);
        auto result = opts.parse(argc, argv);

        if (result.contains("help"))
        {
            // Show only the default group, keeping hidden debug options out.
            std::cout << opts.help({""}) << '\n';
            return 0;
        }

        Config cfg(core::dirs::config(), core::dirs::cache());
        configureLogger(cfg.logDir(), cfg.logFilename());
        trace.emplace("",
            std::format("{:-^80s}", "> START <"),
            std::format("{:-^80s}\n", "> END <"));
        core::utl::logBuildInfo(BuildInfo::Version,
                                BuildInfo::CommitSHA,
                                BuildInfo::Timestamp);

        if (runMetricsReview(result))
        {
            return 0;
        }

        populateConfig(result, cfg);
        logConfig(cfg);

        DuplicateDetector detector;
        Progress progress(&std::cout, cfg.updateFrequency());

        scanDirectories(cfg, detector, progress);
        outputFiles(cfg.allFilesPath(), detector);

        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg.dupFilesPath(), detector);

        // start deletion of the duplicates
        auto strategy = createDeletionStrategy(cfg);

        DeletionState state;
        PathsPersister persisIgn(state.ignored.paths(), cfg.ignFilesPath());
        PathsPersister persisKeep(state.keepFrom.paths(), cfg.keepFilesPath());
        PathsPersister persisDel(state.deleteFrom.paths(), cfg.delFilesPath());

        state.keepFrom.add(cfg.dirsToKeepFrom());
        state.deleteFrom.add(cfg.dirsToDeleteFrom());

        StreamIO io(std::cout, std::cin);
        DeletionContext ctx {*strategy, progress, io, state};

        deleteDuplicates(detector, ctx);
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
