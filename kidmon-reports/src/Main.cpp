#include "Options.h"
#include "QueryBuilder.h"
#include "QueryVisualizer.h"

#include <kidmon/common/Utils.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/FileSystemRepository.h>
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
    conf.topN_ = reportsConf.topN;
    QueryVisualizer queryVis(conf);

    return queryVis;
}

void handleListUsers(const fs::path& reportsDir)
{
    FileSystemRepository repo(reportsDir);
    spdlog::trace("Listing users...");

    std::ostringstream ss;
    repo.queryUsers([&ss](const std::string& username) {
        ss << '\n' << username;
        return true;
    });

    spdlog::info("Users: {}", ss.str());
    spdlog::trace("Listing completed.");
}


void handleQueryUser(const IRepository& repo,
                     const ReportsConfig& conf,
                     QueryVisualizer& queryVisualizer)
{
    validateUser(repo, conf.username);

    const auto queryFilter = buildFilter(conf);
    const auto queryCondition = buildCondition(conf);
    const auto transform = buildTransform(conf);

    std::ostringstream oss;
    queryCondition->write(oss);
    spdlog::info("Query condition: {}", oss.str());

    repo.queryEntries(queryFilter,
                      [&queryVisualizer, &queryCondition, &transform](Entry& entry) {
                          transform->apply(entry);
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
        core::utl::configureLogger(conf.logsDir, conf.logFilename);
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
        if (!result.unmatched().empty())
        {
            spdlog::error("Unexpected argument '{}'", result.unmatched().front());
        }

        if (result.contains("help") || !result.unmatched().empty())
        {
            std::cout << opts.help() << '\n';
            return 2;
        }

        fs::path reportDir(result["reports-dir"].as<std::string>());
        StopWatch sw;
        sw.start();

        if (reportDir.empty())
        {
            reportDir = core::dirs::data().append("kidmon").lexically_normal();
            reportDir /= "reports";

            spdlog::warn("No reports directory is provided, defaulting to '{}'",
                         core::file::path2s(reportDir));
        }

        if (!fs::is_directory(reportDir))
        {
            throw std::invalid_argument(
                std::format("Reports directory '{}' does not exist",
                            core::file::path2s(reportDir)));
        }

        if (result.contains("list"))
        {
            handleListUsers(reportDir);
        }
        else
        {
            const ReportsConfig reportsConf = makeReportsConfig(result);

            FileSystemRepository repo(reportDir);
            QueryVisualizer queryVisualizer = buildVisualizer(reportsConf);

            handleQueryUser(repo, reportsConf, queryVisualizer);
            queryVisualizer.display();
        }

        spdlog::info("Processing took: {}", core::str::humanizeDuration(sw.elapsed()));
    }
    catch (const std::exception& e)
    {
        spdlog::error("exception: {}", e.what());
        return 1;
    }

    return 0;
}


// #include <kidmon/repo/SqliteRepository.h>
// #include <kidmon/repo/RepositoryMigrator.h>

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-function"

// SqliteRepository sqlRepo(core::dirs::current() / "all-activity.db");

// StopWatch sw;
// km::migrate(repo, sqlRepo, [&sw](const MigrationStats& progress) {
//     if (sw.elapsedMs() > 1000) {
//         std::cout << "copied: " << progress.entriesCopied
//                     << "failed: " << progress.entriesFailed << '\r';
//         sw.restart();
//     }
//     return true;
// });

// #pragma GCC diagnostic pop
