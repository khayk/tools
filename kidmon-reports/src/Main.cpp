#include "transform/Transforms.h"
#include "condition/Conditions.h"

#include <core/utils/Log.h>
#include <core/utils/StopWatch.h>

#include <kidmon/common/Tracer.h>
#include <kidmon/common/Utils.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/FileSystemRepository.h>

#include <cxxopts.hpp>
#include <iostream>

namespace {

const std::string_view g_reportsDir =
    "C:/Windows/System32/config/systemprofile/AppData/Local/kidmon/reports";

struct ReportsConfig
{
    uint32_t minutes {0};
    uint32_t hours {0};
    uint32_t days {0};
    uint32_t months {0};
    std::vector<int> range;
    std::vector<std::string> fields;
    std::vector<std::string> titles {};
    std::vector<std::string> processes {};
    std::string username {};
    bool caseInsensitive {false};
};

class QueryVisualizer
{
    StopWatch sw_ {true};
    int numEntries_ = 0;
    int64_t prevUpdate_ {100};   // don't show progress for short queries

public:
    void update()
    {
        ++numEntries_;

        if (sw_.elapsedMs() - prevUpdate_ > 100)
        {
            prevUpdate_ = sw_.elapsedMs();
            std::cout << "Processed: " << numEntries_ << "\r";
        }
    }

    void display(const std::vector<Entry>& entries) const
    {
        std::cout << "Filtered: " << entries.size() << " out of " << numEntries_ << '\n';
    }
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
    maybeGet("user",             res, conf.username);
    maybeGet("minutes",          res, conf.minutes);
    maybeGet("hours",            res, conf.hours);
    maybeGet("days",             res, conf.days);
    maybeGet("months",           res, conf.months);
    maybeGet("range",            res, conf.range);
    maybeGet("fields",           res, conf.fields);
    maybeGet("title",            res, conf.titles);
    maybeGet("process",          res, conf.processes);
    maybeGet("case-insensitive", res, conf.caseInsensitive);
}

TimePoint makeTimepoint(int year,
                        int month,
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


Filter buildFilter(const ReportsConfig& conf)
{
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
        from = (!conf.range.empty()) ? makeTimepoint(conf.range[0]) : TimePoint::min();
        to = (conf.range.size() > 1) ? makeTimepoint(conf.range[1]) : SystemClock::now();
    }

    return Filter(conf.username, from, to);
}

QueryVisualizer buildVisualizer(const ReportsConfig& conf)
{
    std::ignore = conf;
    QueryVisualizer queryVis;
    
    return queryVis;
}

template <typename LogicType>
ConditionPtr combineConditions(std::vector<ConditionPtr>&& conditions)
{
    if (conditions.empty())
    {
        return {};
    }

    ConditionPtr cond = std::move(conditions.back());
    conditions.pop_back();

    if (conditions.empty())
    {
        return cond;
    }

    return std::make_unique<LogicType>(
        combineConditions<LogicType>(std::move(conditions)),
        std::move(cond)
    );
}

template <typename CondType>
std::vector<ConditionPtr> createConditions(const std::vector<std::string>& values)
{
    std::vector<ConditionPtr> conds;
    
    for (const auto& value : values)
    {
        conds.push_back(std::make_unique<CondType>(value));
    }

    return conds;
}

ConditionPtr buildCondition(const ReportsConfig& conf)
{
    std::vector<ConditionPtr> conditions;

    if (!conf.processes.empty())
    {
        conditions.push_back(
            combineConditions<LogicalOR>(
                createConditions<HasProcessCondition>(conf.processes)
            )
        );
    }

    if (!conf.titles.empty())
    {
        conditions.push_back(
            combineConditions<LogicalOR>(
                createConditions<HasTitleCondition>(conf.titles)
            )
        );
    }
    
    return combineConditions<LogicalAND>(std::move(conditions));
}

TransformPtr buildTransform(const ReportsConfig& conf)
{
    std::vector<TransformPtr> transformers;

    if (conf.caseInsensitive)
    {
        if (!conf.processes.empty())
        {
            transformers.push_back(std::make_unique<ProcessPathToLowerTransform>());
        }

        if (!conf.titles.empty())
        {
            transformers.push_back(std::make_unique<TitleToLowerTransform>());
        }
    }

    return std::make_unique<SpreadTransform>(std::move(transformers));
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

    const auto queryFilter    = buildFilter(conf);
    const auto queryCondition = buildCondition(conf);
    const auto transform      = buildTransform(conf);
    
    std::ostringstream oss;
    queryCondition->write(oss);
    
    spdlog::info("Query condition: {}", oss.str());

    QueryVisualizer queryVisualizer = buildVisualizer(conf);
    std::vector<Entry> filtered;
    
    repo.queryEntries(
        queryFilter,
        [&queryVisualizer, &filtered, &queryCondition, &transform](Entry& entry) {
            queryVisualizer.update();

            transform->apply(entry);

            if (queryCondition->met(entry))
            {
                filtered.push_back(entry);
            }

            return true;
        });

    queryVisualizer.display(filtered);
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
            ("c,case-insensitive", "Enables case-insensitive search")
            ("l,list", "Lists available users")
            ("u,user", "The name of the user to be queried", cxxopts::value<std::string>())
            ("m,minutes", "The last 'm' minutes", cxxopts::value<uint32_t>())
            ("h,hours", "The last 'h' hours", cxxopts::value<uint32_t>())
            ("d,days", "The last 'd' days", cxxopts::value<uint32_t>())
            ("M,months", "The last 'M' months", cxxopts::value<uint32_t>())
            ("r,range", "The dates range (ex: 20240913,20241030)", cxxopts::value<std::vector<int>>())
            ("f,fields", "The fields to be displayed", cxxopts::value<std::vector<std::string>>()->default_value(""))
            ("t,title", "The window titles", cxxopts::value<std::vector<std::string>>())
            ("p,process", "The process names", cxxopts::value<std::vector<std::string>>())
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
            std::cout << opts.help() << '\n';
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

        spdlog::info("Processing took: {}", utl::humanizeDuration(sw.elapsed()));
    }
    catch (const std::exception& e)
    {
        spdlog::error("exception: {}", e.what());
        return 1;
    }

    return 0;
}