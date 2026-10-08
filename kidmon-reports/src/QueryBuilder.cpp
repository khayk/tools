#include "QueryBuilder.h"
#include "condition/Conditions.h"

#include <kidmon/common/Utils.h>

#include <chrono>
#include <ctime>
#include <format>
#include <stdexcept>

namespace km::reports {

namespace {

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

TimePoint makeTimepoint(const std::chrono::year_month_day& ymd)
{
    return makeTimepoint(static_cast<uint32_t>(static_cast<int>(ymd.year())),
                         static_cast<uint32_t>(ymd.month()),
                         static_cast<uint32_t>(ymd.day()));
}

/**
 * Parses a date from the input expecting it to be an integer the format YYYYMMDD
 */
std::chrono::year_month_day parseDate(uint32_t date)
{
    const std::chrono::year_month_day ymd {
        std::chrono::year(static_cast<int>(date / 10'000)),
        std::chrono::month(date / 100 % 100),
        std::chrono::day(date % 100)};

    if (!ymd.ok() || ymd.year() < std::chrono::year(1970))
    {
        throw std::invalid_argument(
            std::format("Invalid date '{}', expected YYYYMMDD", date));
    }

    return ymd;
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
std::vector<ConditionPtr> createConditions(const std::vector<std::string>& values,
                                           bool caseSensitive)
{
    std::vector<ConditionPtr> conds;
    conds.reserve(values.size());

    for (const auto& value : values)
    {
        conds.push_back(std::make_unique<CondType>(value, caseSensitive));
    }

    return conds;
}


ConditionPtr buildExcludeCondition(const ReportsConfig& conf)
{
    std::vector<ConditionPtr> conditions;

    if (!conf.excludeProcesses.empty())
    {
        for (auto& cond : createConditions<HasProcessCondition>(conf.excludeProcesses,
                                                                conf.caseSensitive))
        {
            conditions.push_back(std::move(cond));
        }
    }

    if (!conf.excludeTitles.empty())
    {
        for (auto& cond : createConditions<HasTitleCondition>(conf.excludeTitles,
                                                              conf.caseSensitive))
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
            createConditions<HasProcessCondition>(conf.processes,
                                                  conf.caseSensitive)));
    }

    if (!conf.titles.empty())
    {
        conditions.push_back(combineConditions<LogicalOR>(
            createConditions<HasTitleCondition>(conf.titles, conf.caseSensitive)));
    }

    if (conditions.empty())
    {
        return std::make_unique<TrueCondition>();
    }

    return combineConditions<LogicalAND>(std::move(conditions));
}

} // namespace


void validateUser(const IRepository& repo, const std::string& username)
{
    if (username.empty())
    {
        throw std::invalid_argument("No user is given, use --user (see --list)");
    }

    bool found = false;
    repo.queryUsers([&found, &username](const std::string& user) {
        found = (user == username);
        return !found;
    });

    if (!found)
    {
        throw std::invalid_argument(
            std::format("Unknown user '{}' (see --list)", username));
    }
}

Filter buildFilter(const ReportsConfig& conf)
{
    TimePoint to = SystemClock::now();
    TimePoint from = to;

    if (!conf.range.empty())
    {
        if (conf.range.size() > 2)
        {
            throw std::invalid_argument("The range expects at most two dates");
        }

        from = makeTimepoint(parseDate(conf.range[0]));

        if (conf.range.size() > 1)
        {
            // The end date is inclusive, so stop at the start of the next day
            const auto last = std::chrono::sys_days(parseDate(conf.range[1]));
            to = makeTimepoint(
                std::chrono::year_month_day(last + std::chrono::days(1)));
        }

        if (from >= to)
        {
            throw std::invalid_argument("The range start must precede its end");
        }
    }
    else if (conf.minutes == 0 && conf.hours == 0 && conf.days == 0 &&
             conf.months == 0)
    {
        // No time option is given, report today
        const auto now = km::utl::timet2tm(SystemClock::to_time_t(to));
        from = makeTimepoint(static_cast<uint32_t>(now.tm_year + 1900),
                             static_cast<uint32_t>(now.tm_mon + 1),
                             static_cast<uint32_t>(now.tm_mday));
    }
    else
    {
        from -= std::chrono::minutes(conf.minutes);
        from -= std::chrono::hours(conf.hours);
        from -= std::chrono::days(conf.days);
        from -= std::chrono::months(conf.months);
    }

    return Filter(conf.username, from, to);
}

ConditionPtr buildCondition(const ReportsConfig& conf)
{
    auto excludeCondition = std::make_unique<Negate>(buildExcludeCondition(conf));
    auto includeCondition = buildIncludeCondition(conf);

    return std::make_unique<LogicalAND>(std::move(excludeCondition),
                                        std::move(includeCondition));
}


} // namespace km::reports
