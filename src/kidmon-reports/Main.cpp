#include "aggregate/Aggregate.h"
#include "aggregate/Predicate.h"
#include "transform/Transforms.h"
#include "condition/Conditions.h"

#include <core/utils/Log.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/StopWatch.h>

#include <kidmon/common/Tracer.h>
#include <kidmon/common/Utils.h>
#include <kidmon/config/Config.h>
#include <kidmon/repo/FileSystemRepository.h>

#include <cxxopts.hpp>
#include <iostream>
#include <numeric>
#include <set>

namespace {

const std::string_view g_reportsDir =
#ifdef _WIN32
    "C:"
#else
    "/mnt/c"
#endif
    "/Windows/System32/config/systemprofile/AppData/Local/kidmon/reports";

struct ReportsConfig
{
    uint32_t minutes {0};
    uint32_t hours {0};
    uint32_t days {0};
    uint32_t months {0};
    uint32_t topN {0};
    std::vector<uint32_t> range;
    std::vector<std::string> fields;
    std::vector<std::string> titles {};
    std::vector<std::string> processes {};
    std::vector<std::string> excludeTitles {};
    std::vector<std::string> excludeProcesses {};
    std::string username {};
    bool caseInsensitive {false};
};

class QueryVisualizer
{
    std::wstring buf_;
    AggregatePtr pathByNameAggr_;

public:
    struct Config
    {
        std::vector<std::string> fields_;
        uint32_t topN_ {10};
    };

    QueryVisualizer(Config conf)
        : conf_(std::move(conf))
    {
        // using SplitterAggr = Splitter<ProcPathAggr, TitleAggr>;
        using TitleAggr = Aggregate<Data, TitleBuilder>;
        using ProcPathAggr = Aggregate<TitleAggr, ProcPathBuilder>;
        using ProcNameAggr = Aggregate<ProcPathAggr, ProcNameBuilder>;

        pathByNameAggr_ = std::make_unique<ProcNameAggr>();
    }

    void update(const Entry&)
    {
        ++numEntries_;

        if (sw_.elapsedMs() - prevUpdate_ > 100)
        {
            prevUpdate_ = sw_.elapsedMs();
            std::cout << "Processed: " << numEntries_ << "\r" << std::flush;
        }
    }

    void add(const Entry& entry)
    {
        auto procname = file::path2s(entry.processInfo.processPath.filename());
        str::utf8LowerInplace(procname, &buf_);

        pathByNameAggr_->update(entry);
    }

    void display() const
    {
        std::cout << "Filtered: " << entries_.size() << " out of " << numEntries_
                  << '\n';

        // pathByNameAggr_->write(std::cout, conf_.topN_, 0);
        // pathByNameAggr_->write(std::cout, 0);

        pathByNameAggr_->enumarate(
            conf_.topN_,
            0,
            [](std::string_view field,
               std::string_view value,
               uint32_t depth,
               const Data& data) {
                if (value.empty() && depth != 0)
                {
                    return;
                }

                std::cout << std::string(4 * depth, ' ') << field;

                if (!value.empty())
                {
                    std::cout << ": " << value << ", ";
                }

                std::cout << "duration: " << utl::humanizeDuration(data.duration())
                          << '\n';
            });
    }

private:
    Config conf_;
    StopWatch sw_ {true};
    int numEntries_ {};
    int64_t prevUpdate_ {100}; // don't show progress for short queries
    std::vector<Entry> entries_;
};


template <typename T>
void maybeGet(const std::string& name, const cxxopts::ParseResult& res, T& dest)
{
    if (res.count(name) > 0)
    {
        dest = res[name].as<T>();
    }
}

void makeLowercase(std::vector<std::string>& data)
{
    std::wstring wstr;

    for (auto& str : data)
    {
        str::utf8LowerInplace(str, &wstr);
    }
}

void applyCaseTransform(ReportsConfig& conf)
{
    if (conf.caseInsensitive)
    {
        makeLowercase(conf.titles);
        makeLowercase(conf.processes);
    }
}

void initReportsConf(const cxxopts::ParseResult& res, ReportsConfig& conf)
{
    maybeGet("user", res, conf.username);
    maybeGet("minutes", res, conf.minutes);
    maybeGet("hours", res, conf.hours);
    maybeGet("days", res, conf.days);
    maybeGet("months", res, conf.months);
    maybeGet("range", res, conf.range);
    maybeGet("fields", res, conf.fields);
    maybeGet("title", res, conf.titles);
    maybeGet("process", res, conf.processes);
    maybeGet("top", res, conf.topN);
    maybeGet("case-insensitive", res, conf.caseInsensitive);
    maybeGet("exclude-process", res, conf.excludeProcesses);
    maybeGet("exclude-title", res, conf.excludeTitles);
}

TimePoint makeTimepoint(uint32_t year,
                        uint32_t month,
                        uint32_t day = 1,
                        uint32_t hour = 0,
                        uint32_t min = 0,
                        uint32_t sec = 0)
{
    std::tm tm = {};
    tm.tm_sec = static_cast<int>(sec);
    tm.tm_min = static_cast<int>(min);
    tm.tm_hour = static_cast<int>(hour);
    tm.tm_mday = static_cast<int>(day);
    tm.tm_mon = static_cast<int>(month - 1);
    tm.tm_year = static_cast<int>(year - 1900);
    tm.tm_isdst = -1; // Use DST value from local time zone

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

/**
 * Creates TimePoint from the input expecting it to be an integer the format YYYYMMDD
 */
TimePoint makeTimepoint(uint32_t date)
{
    uint32_t day = date % 100;
    date /= 100;
    uint32_t month = date % 100;
    date /= 100;
    uint32_t year = date;

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
        to = (conf.range.size() > 1) ? makeTimepoint(conf.range[1])
                                     : SystemClock::now();
    }

    return Filter(conf.username, from, to);
}

QueryVisualizer buildVisualizer(const ReportsConfig& reportsConf)
{
    QueryVisualizer::Config conf;
    conf.topN_ = reportsConf.topN;
    QueryVisualizer queryVis(conf);

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
        std::move(cond));
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


ConditionPtr buildExcludeCondition(const ReportsConfig& conf)
{
    std::vector<ConditionPtr> conditions;

    if (!conf.excludeProcesses.empty())
    {
        for (auto& cond : createConditions<HasProcessCondition>(conf.excludeProcesses))
        {
            conditions.push_back(std::move(cond));
        }
    }

    if (!conf.excludeTitles.empty())
    {
        for (auto& cond : createConditions<HasTitleCondition>(conf.excludeTitles))
        {
            conditions.push_back(std::move(cond));
        }
    }

    if (conditions.empty())
    {
        return std::make_unique<FalseCondition>();
    }

    return combineConditions<LogicalOR>(std::move(conditions));
}

ConditionPtr buildIncludeCondition(const ReportsConfig& conf)
{
    std::vector<ConditionPtr> conditions;

    if (!conf.processes.empty())
    {
        conditions.push_back(combineConditions<LogicalOR>(
            createConditions<HasProcessCondition>(conf.processes)));
    }

    if (!conf.titles.empty())
    {
        conditions.push_back(combineConditions<LogicalOR>(
            createConditions<HasTitleCondition>(conf.titles)));
    }

    if (conditions.empty())
    {
        return std::make_unique<TrueCondition>();
    }

    return combineConditions<LogicalAND>(std::move(conditions));
}

ConditionPtr buildCondition(const ReportsConfig& conf)
{
    auto excludeCondition = std::make_unique<Negate>(buildExcludeCondition(conf));
    auto includeCondition = buildIncludeCondition(conf);

    return std::make_unique<LogicalAND>(std::move(excludeCondition),
                                        std::move(includeCondition));
}


TransformPtr buildTransform(const ReportsConfig& conf)
{
    std::vector<TransformPtr> transformers;

    if (conf.caseInsensitive)
    {
        if (!conf.processes.empty() || !conf.excludeProcesses.empty())
        {
            transformers.push_back(std::make_unique<ProcessPathToLowerTransform>());
        }

        if (!conf.titles.empty() || !conf.excludeTitles.empty())
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


void handleQueryUser(const IRepository& repo,
                     const ReportsConfig& conf,
                     QueryVisualizer& queryVisualizer)
{
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
    std::optional<ScopedTrace> trace;

    try
    {
        cxxopts::Options opts("kidmon-reports", "Produce reports for user activity");

        // clang-format off
        opts.add_options()
            ("c,case-insensitive", "Enables case-insensitive search")
            ("l,list", "Lists available users")
            ("u,user", "The name of the user to be queried", cxxopts::value<std::string>())
            ("m,minutes", "The last 'm' minutes", cxxopts::value<uint32_t>())
            ("h,hours", "The last 'h' hours", cxxopts::value<uint32_t>())
            ("d,days", "The last 'd' days", cxxopts::value<uint32_t>())
            ("M,months", "The last 'M' months", cxxopts::value<uint32_t>())
            ("r,range", "The dates range (ex: 20240913,20241030)", cxxopts::value<std::vector<uint32_t>>())
            ("f,fields", "The fields to be displayed", cxxopts::value<std::vector<std::string>>()->default_value(""))
            ("t,title", "The window titles", cxxopts::value<std::vector<std::string>>())
            ("p,process", "The process names", cxxopts::value<std::vector<std::string>>())
            ("T,top", "The top N results", cxxopts::value<uint32_t>()->default_value("10"))
            ("exclude-process", "The process names to exclude", cxxopts::value<std::vector<std::string>>())
            ("exclude-title", "The window titles to exclude", cxxopts::value<std::vector<std::string>>())
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

            initReportsConf(result, reportsConf);
            applyCaseTransform(reportsConf);

            FileSystemRepository repo(g_reportsDir);
            QueryVisualizer queryVisualizer = buildVisualizer(reportsConf);

            handleQueryUser(repo, reportsConf, queryVisualizer);
            queryVisualizer.display();
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