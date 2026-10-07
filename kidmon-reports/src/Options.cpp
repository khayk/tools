#include "Options.h"

#include <core/utils/Str.h>

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
        ("reports-dir", "The reports directory", cxxopts::value<std::string>()->default_value(""))
        ("e,help", "Print usage")
    ;
    // clang-format on

    return opts;
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
    maybeGet("fields", res, conf.fields);
    maybeGet("title", res, conf.titles);
    maybeGet("process", res, conf.processes);
    // Read directly: contains() ignores default values
    conf.topN = res["top"].as<uint32_t>();
    maybeGet("case-sensitive", res, conf.caseSensitive);
    maybeGet("exclude-process", res, conf.excludeProcesses);
    maybeGet("exclude-title", res, conf.excludeTitles);

    applyCaseTransform(conf);

    return conf;
}

} // namespace km::reports
