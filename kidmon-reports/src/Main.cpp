#include <core/utils/Log.h>
#include <core/utils/StopWatch.h>

#include <kidmon/common/Tracer.h>
#include <kidmon/common/Utils.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/FileSystemRepository.h>

#include <cxxopts.hpp>
#include <iostream>

namespace {

const fs::path g_reportsDir =
    "C:/Windows/System32/config/systemprofile/AppData/Local/kidmon/reports";

struct ReportsConfig
{
    uint32_t minutes {0};
    uint32_t hours {0};
    uint32_t days {0};
    uint32_t months {0};
    std::vector<int> range;
    std::vector<std::string> fields;
    std::string title {};
    std::string process {};
    std::string username {};
    bool caseSensitiveLookup {false};
};

template <typename T>
void maybeGet(const std::string& name, const cxxopts::ParseResult& res, T& dest)
{
    if (res.count(name) > 0)
    {
        dest = res[name].as<T>();
    }
}

void reportsConfigFromOpts(const cxxopts::ParseResult& res, ReportsConfig& conf)
{
    maybeGet("user",           res, conf.username);
    maybeGet("minutes",        res, conf.minutes);
    maybeGet("hours",          res, conf.hours);
    maybeGet("days",           res, conf.days);
    maybeGet("months",         res, conf.months);
    maybeGet("range",          res, conf.range);
    maybeGet("fields",         res, conf.fields);
    maybeGet("title",          res, conf.title);
    maybeGet("process",        res, conf.process);
    maybeGet("case-sensitive", res, conf.caseSensitiveLookup);
}

TimePoint makeTimepoint(int year = 2024,
                        int month = 1,
                        int day = 1,
                        int hour = 0,
                        int min = 0,
                        int sec = 0)
{
    std::tm tm = {};
    tm.tm_sec = sec;
    tm.tm_min = min;
    tm.tm_hour = hour;
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1; // Use DST value from local time zone

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

/**
 * Creates TimePoint from the input expecting it to be an integer the format YYYYMMDD
 */
TimePoint makeTimepoint(uint32_t date)
{
    int day = date % 100;
    date /= 100;
    int month = date % 100;
    date /= 100;
    int year = date;

    return makeTimepoint(year, month, day);
}


bool validateArguments(const cxxopts::ParseResult& result)
{
    std::ignore = result;
    // @todo:hayk - add validation logic

    return true;
}

void handleListUsers()
{
    FileSystemRepository repo(g_reportsDir);
    spdlog::trace("Listing users...");

    std::ostringstream ss;
    repo.queryUsers([&ss](const std::string& username) {
        ss << '\n' << username;
        return true;
    });

    spdlog::info("Users: {}", ss.str());
    spdlog::trace("Listing completed.");
}


void handleQueryUser(const ReportsConfig& conf)
{
    FileSystemRepository repo(g_reportsDir);
    TimePoint to = SystemClock::now();
    TimePoint from = to;

    if (conf.range.empty())
    {
        from -= std::chrono::minutes(conf.minutes);
        from -= std::chrono::hours(conf.hours);
        from -= std::chrono::days(conf.days);
        from -= std::chrono::months(conf.months);
    }
    else
    {
        from = (conf.range.size() > 0) ? makeTimepoint(conf.range[0]) : TimePoint::min();
        to = (conf.range.size() > 1) ? makeTimepoint(conf.range[1]) : SystemClock::now();
    }

    Filter filter(conf.username, from, to);

    StopWatch sw(true);
    int64_t prevUpdate {100};   // skip logging the status for short queries
    int numEntries = 0;

    repo.queryEntries(filter, [&sw, &numEntries, &prevUpdate, &conf](const Entry& entry) {
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
    std::optional<ScopedTrace> trace;

    try
    {
        cxxopts::Options opts("kidmon-reports", "Produce reports for the user activity");

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

        AppConfig conf;
        conf.logFilename = "kidmon-reports.log";
        utl::configureLogger(conf.logsDir, conf.logFilename);

        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));

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
            ReportsConfig reportsConf;
            reportsConfigFromOpts(result, reportsConf);
            handleQueryUser(reportsConf);
        }

        spdlog::info("Processing took: {}ms", utl::humanizeDuration(sw.elapsed()));
    }
    catch (const std::exception& e)
    {
        spdlog::error("exception: {}", e.what());
        return 1;
    }

    return 0;
}