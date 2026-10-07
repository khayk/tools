#include "QueryBuilder.h"
#include "condition/Conditions.h"
#include "transform/Transforms.h"

#include <ctime>

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
    conds.reserve(values.size());

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

} // namespace


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

    if (!conf.caseSensitive)
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

} // namespace km::reports
