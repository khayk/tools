#include "Options.h"
#include "QueryBuilder.h"
#include "QueryVisualizer.h"

#include <kidmon/common/Utils.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/RepositoryFactory.h>
#include <kidmon/repo/RepositoryMigrator.h>
#include <BuildInfo.h>

#include <core/utils/Dirs.h>
#include <core/utils/Log.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/StopWatch.h>
#include <core/utils/ScopedLog.h>
#include <spdlog/spdlog.h>

#include <cxxopts.hpp>
#include <iostream>
#include <sstream>

using namespace km;
using namespace km::reports;

namespace {

QueryVisualizer buildVisualizer(const ReportsConfig& reportsConf)
{
    QueryVisualizer::Config conf;
    conf.top_ = reportsConf.top;
    QueryVisualizer queryVis(conf);

    return queryVis;
}

void handleListUsers(const IRepository& repo)
{
    spdlog::trace("Listing users...");

    repo.queryUsers([](const std::string& username) {
        std::cout << username << '\n';
        return true;
    });

    spdlog::trace("Listing completed.");
}

/**
 * Copies all entries from the source into the target repository. Returns false if
 * any entry failed.
 */
bool handleMigrate(const IRepository& source,
                   const RepositorySpec& sourceSpec,
                   const RepositorySpec& targetSpec)
{
    validateMigration(sourceSpec, targetSpec);
    const auto destination = openRepository(targetSpec, RepositoryAccess::Write);

    spdlog::info("Migrating '{}' into '{}'",
                 toString(sourceSpec),
                 toString(targetSpec));

    StopWatch sw(true);
    const auto stats =
        migrate(source, *destination, [&sw](const MigrationStats& progress) {
            if (sw.elapsedMs() > 1000)
            {
                spdlog::info("Migrated {} entries, {} failed",
                             progress.entriesCopied,
                             progress.entriesFailed);
                sw.restart();
            }
            return true;
        });

    std::cout << std::format("Users: {}, entries migrated: {}, failed: {}\n",
                             stats.usersSeen,
                             stats.entriesCopied,
                             stats.entriesFailed);

    return stats.entriesFailed == 0;
}

void handleQueryUser(const IRepository& repo,
                     const ReportsConfig& conf,
                     QueryVisualizer& queryVisualizer)
{
    validateUser(repo, conf.username);

    const auto queryFilter = buildFilter(conf);
    const auto queryCondition = buildCondition(conf);

    std::ostringstream oss;
    queryCondition->write(oss);
    spdlog::info("Query condition: {}", oss.str());

    repo.queryEntries(queryFilter,
                      [&queryVisualizer, &queryCondition](const Entry& entry) {
                          queryVisualizer.update(entry);

                          if (queryCondition->met(entry))
                          {
                              queryVisualizer.add(entry);
                          }

                          return true;
                      });
}

} // namespace

int main(int argc, char* argv[])
{
    std::optional<ScopedLog> appLog;

    try
    {
        auto opts = makeOptions();

        AppConfig conf;
        conf.logFilename = "kidmon-reports.log";
        // Console logs go to stderr, keeping stdout for the program's output
        core::utl::configureLogger(conf.logsDir,
                                   conf.logFilename,
                                   spdlog::level::trace,
                                   core::utl::ConsoleStream::Stderr);
        appLog.emplace("",
                       spdlog::level::info,
                       std::format("{:-^80s}", "> START <"),
                       std::format("{:-^80s}\n", "> END <"));
        core::utl::logBuildInfo(BuildInfo::Version,
                                BuildInfo::CommitSHA,
                                BuildInfo::Timestamp);

        const auto result = opts.parse(argc, argv);

        // cxxopts silently ignores stray positional arguments, e.g. the value in
        // "-title foo" (parsed as "-t itle foo")
        auto error = findOptionConflict(result);
        if (!result.unmatched().empty())
        {
            error =
                std::format("Unexpected argument '{}'", result.unmatched().front());
        }

        if (error)
        {
            spdlog::error("{}", *error);
        }

        if (result.contains("help") || error)
        {
            std::cout << opts.help() << '\n';
            return 2;
        }

        StopWatch sw;
        sw.start();

        RepositorySpec sourceSpec;
        if (result.contains("source"))
        {
            sourceSpec = parseRepositorySpec(result["source"].as<std::string>());
        }
        else
        {
            // The folder kidmon writes its reports to
            sourceSpec.location = core::dirs::data() / "kidmon" / "reports";
        }

        spdlog::info("Source: '{}'", toString(sourceSpec));
        const auto source = openRepository(sourceSpec, RepositoryAccess::Read);

        int exitCode = 0;

        if (result.contains("list"))
        {
            handleListUsers(*source);
        }
        else if (result.contains("migrate-to"))
        {
            const auto targetSpec =
                parseRepositorySpec(result["migrate-to"].as<std::string>());
            exitCode = handleMigrate(*source, sourceSpec, targetSpec) ? 0 : 1;
        }
        else
        {
            const ReportsConfig reportsConf = makeReportsConfig(result);
            QueryVisualizer queryVisualizer = buildVisualizer(reportsConf);

            handleQueryUser(*source, reportsConf, queryVisualizer);
            queryVisualizer.display();
        }

        spdlog::info("Processing took: {}", core::str::humanizeDuration(sw.elapsed()));

        return exitCode;
    }
    catch (const std::exception& e)
    {
        spdlog::error("exception: {}", e.what());
        return 1;
    }

    return 0;
}
