#pragma once

#include <Options.h>

#include <optional>
#include <string>
#include <vector>

namespace km::reports::test {

inline cxxopts::ParseResult parse(std::vector<std::string> args)
{
    args.insert(args.begin(), "kidmon-reports");

    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args)
    {
        argv.push_back(arg.c_str());
    }

    auto opts = makeOptions();
    return opts.parse(static_cast<int>(argv.size()), argv.data());
}

inline ReportsConfig parseArgs(std::vector<std::string> args)
{
    return makeReportsConfig(parse(std::move(args)));
}

inline std::optional<std::string> optionConflict(std::vector<std::string> args)
{
    return findOptionConflict(parse(std::move(args)));
}

} // namespace km::reports::test
