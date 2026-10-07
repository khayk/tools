#pragma once

#include <Options.h>

#include <string>
#include <vector>

namespace km::reports::test {

inline ReportsConfig parseArgs(std::vector<std::string> args)
{
    args.insert(args.begin(), "kidmon-reports");

    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args)
    {
        argv.push_back(arg.c_str());
    }

    auto opts = makeOptions();
    const auto result = opts.parse(static_cast<int>(argv.size()), argv.data());

    return makeReportsConfig(result);
}

} // namespace km::reports::test
