#include <duplicates/DuplicateDetector.h>
#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DuplicateOperation.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Config.h>
#include <duplicates/Node.h>
#include <duplicates/CmdLine.h>

#include <core/utils/Log.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Tracer.h>
#include <core/utils/Sys.h>
#include <core/utils/Str.h>
#include <core/utils/Dirs.h>

#include <fmt/format.h>
#include <cxxopts.hpp>

#include <memory>
#include <system_error>

using tools::utl::configureLogger;

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
            std::cout << opts.help() << '\n';
            return 0;
        }

        Config cfg(dirs::config(), dirs::cache());
        configureLogger(cfg.logDir(), cfg.logFilename());
        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
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

        DeletionConfig deletionCfg {*strategy, std::cout, std::cin, progress};
        PathsPersister persisIgn(deletionCfg.ignoredPaths().paths(), cfg.ignFilesPath());
        PathsPersister persisKeep(deletionCfg.keepFromPaths().paths(), cfg.keepFilesPath());
        PathsPersister persisDel(deletionCfg.deleteFromPaths().paths(), cfg.delFilesPath());

        deletionCfg.keepFromPaths().add(cfg.dirsToKeepFrom());
        deletionCfg.deleteFromPaths().add(cfg.dirsToDeleteFrom());

        deleteDuplicates(detector, deletionCfg);
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
