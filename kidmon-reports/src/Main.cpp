#include <core/utils/Log.h>
#include <core/utils/StopWatch.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/FileSystemRepository.h>

#include <cxxopts.hpp>
#include <iostream>

namespace {

const fs::path g_reportsDir =
    "C:/Windows/System32/config/systemprofile/AppData/Local/kidmon/reports";

TimePoint makeTimepoint(int year = 2024,
                        int month = 1,
                        int day = 1,
                        int hour = 0,
                        int min = 0,
                        int sec = 0)
{
    std::tm tm = {
        /* .tm_sec  = */ sec,
        /* .tm_min  = */ min,
        /* .tm_hour = */ hour,
        /* .tm_mday = */ day,
        /* .tm_mon  = */ month - 1,
        /* .tm_year = */ year - 1900,
    };
    tm.tm_isdst = -1; // Use DST value from local time zone

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

/// List users
///      - users
/// Query user
///      - has title
///      - has process
///      - by date
///      - by custom range
///      - by month
///      - from last M minutes
///      - from last N hours
/// Options
///      - case sensitivity control
///


bool validateArguments(const cxxopts::ParseResult& result)
{
    std::ignore = result;

    return true;
}

void handleListUsers()
{
    FileSystemRepository repo(g_reportsDir);
    spdlog::trace("Listing users...");

    repo.queryUsers([](const std::string& username) {
        std::cout << username << '\n';
        return true;
    });

    spdlog::trace("Listing completed.");
}


void handleQueryUser(const StopWatch& sw, const std::string& username)
{
    FileSystemRepository repo(g_reportsDir);

    // @todo:hayk - extend input argument and init filter from input
    const TimePoint from = makeTimepoint(2024, 1, 22, 0, 0, 0);
    const TimePoint to = makeTimepoint(2024, 9, 25, 0, 0, 0);
    Filter filter(username, from, to);

    int64_t prevUpdate {};
    int numEntries = 0;
    repo.queryEntries(filter, [&sw, &numEntries, &prevUpdate](const Entry& entry) {
        if (sw.elapsedMs() - prevUpdate > 100)
        {
            prevUpdate = sw.elapsedMs();
            std::cout << "Processed: " << numEntries << "\r";
        }

        std::ignore = entry;

        ++numEntries;
        return true;
    });
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        AppConfig conf;
        conf.logFilename = "kidmon-reports.log";
        utl::configureLogger(conf.logsDir, conf.logFilename);

        cxxopts::Options opts("kidmon-reports",
                              "Produce reports for the user activity");

        // clang-format off
        opts.add_options()
            ("c,case-sensitive", "Enables case-sensitive search", cxxopts::value<bool>()->default_value("false"))
            ("l,list", "Lists available users")
            ("u,user", "The name of the user to be queried", cxxopts::value<std::string>())
            ("m,minutes", "The last 'm' minutes", cxxopts::value<uint32_t>())
            ("h,hours", "The last 'h' hours", cxxopts::value<uint32_t>())
            ("d,days", "The last 'd' days", cxxopts::value<uint32_t>())
            ("M,months", "The last 'M' months", cxxopts::value<uint32_t>())
            ("r,range", "The dates range (ex: 20240913,20241030)", cxxopts::value<std::vector<int>>())
            ("f,fields", "The fields to be displayed", cxxopts::value<std::vector<std::string>>()->default_value(""))
            ("t,title", "The window title", cxxopts::value<std::string>())
            ("p,process", "The process name", cxxopts::value<std::string>())
            ("e,help", "Print usage")
        ;
        // clang-format on

        const auto result = opts.parse(argc, argv);

        if (result.count("help") || !validateArguments(result))
        {
            std::cout << opts.help() << std::endl;
            return 2;
        }


        StopWatch sw;
        sw.start();

        if (result.count("list"))
        {
            handleListUsers();
        }
        else
        {
            handleQueryUser(sw, result["user"].as<std::string>());
        }

        spdlog::info("Processing took: {}ms", sw.elapsedMs());
    }
    catch (const std::exception& e)
    {
        spdlog::error("exception: {}", e.what());
        return 1;
    }

    return 0;
}