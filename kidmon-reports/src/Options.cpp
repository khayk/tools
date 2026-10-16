#include "Options.h"

#include <core/utils/Str.h>

#include <format>

namespace km::reports {

namespace {

template <typename T>
void maybeGet(const std::string& name, const cxxopts::ParseResult& res, T& dest)
{
    if (res.contains(name))
    {
        dest = res[name].as<T>();
    }
}

void makeLowercase(std::vector<std::string>& data)
{
    std::wstring wstr;

    for (auto& str : data)
    {
        core::str::utf8LowerInplace(str, &wstr);
    }
}

} // namespace

cxxopts::Options makeOptions()
{
    cxxopts::Options opts("kidmon-reports", "Produce reports for user activity");

    // clang-format off
    opts.add_options()
        ("c,case-sensitive", "Enables case-sensitive search")
        ("l,list", "Lists available users")
        ("u,user", "The name of the user to be queried", cxxopts::value<std::string>())
        ("m,minutes", "The last 'm' minutes", cxxopts::value<uint32_t>())
        ("H,hours", "The last 'H' hours", cxxopts::value<uint32_t>())
        ("d,days", "The last 'd' days", cxxopts::value<uint32_t>())
        ("M,months", "The last 'M' months", cxxopts::value<uint32_t>())
        ("r,range", "The dates range (ex: 20240913,20241030)", cxxopts::value<std::vector<uint32_t>>())
        ("t,title", "The window titles", cxxopts::value<std::vector<std::string>>())
        ("p,process", "The process names", cxxopts::value<std::vector<std::string>>())
        ("T,top", "The top N per level: processes,paths,titles, 0 hides the level (ex: 10,3)", cxxopts::value<std::vector<uint32_t>>()->default_value("10"))
        ("exclude-process", "The process names to exclude", cxxopts::value<std::vector<std::string>>())
        ("exclude-title", "The window titles to exclude", cxxopts::value<std::vector<std::string>>())
        ("source", "The repository to read from, [fs:|sqlite:]location (default: the kidmon reports folder)", cxxopts::value<std::string>())
        ("migrate-to", "Copy all data from the source into this repository, [fs:|sqlite:]location", cxxopts::value<std::string>())
        ("h,help", "Print usage")
    ;
    // clang-format on

    return opts;
}

std::optional<std::string> findOptionConflict(const cxxopts::ParseResult& res)
{
    if (res.contains("list") && res.contains("migrate-to"))
    {
        return "--list and --migrate-to can't be used together";
    }

    const auto* command = res.contains("list")         ? "--list"
                          : res.contains("migrate-to") ? "--migrate-to"
                                                       : nullptr;
    if (command == nullptr)
    {
        return std::nullopt;
    }

    for (const auto* option : {"user",
                               "minutes",
                               "hours",
                               "days",
                               "months",
                               "range",
                               "title",
                               "process",
                               "exclude-process",
                               "exclude-title",
                               "top",
                               "case-sensitive"})
    {
        if (res.contains(option))
        {
            return std::format("--{} can't be used with {}", option, command);
        }
    }

    return std::nullopt;
}

void applyCaseTransform(ReportsConfig& conf)
{
    if (!conf.caseSensitive)
    {
        makeLowercase(conf.titles);
        makeLowercase(conf.processes);
        makeLowercase(conf.excludeTitles);
        makeLowercase(conf.excludeProcesses);
    }
}

ReportsConfig makeReportsConfig(const cxxopts::ParseResult& res)
{
    ReportsConfig conf;

    maybeGet("user", res, conf.username);
    maybeGet("minutes", res, conf.minutes);
    maybeGet("hours", res, conf.hours);
    maybeGet("days", res, conf.days);
    maybeGet("months", res, conf.months);
    maybeGet("range", res, conf.range);
    maybeGet("title", res, conf.titles);
    maybeGet("process", res, conf.processes);
    // Read directly: contains() ignores default values
    conf.top = res["top"].as<std::vector<uint32_t>>();
    maybeGet("case-sensitive", res, conf.caseSensitive);
    maybeGet("exclude-process", res, conf.excludeProcesses);
    maybeGet("exclude-title", res, conf.excludeTitles);

    applyCaseTransform(conf);

    return conf;
}

} // namespace km::reports
