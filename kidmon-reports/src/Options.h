#pragma once

#include <cxxopts.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace km::reports {

struct ReportsConfig
{
    uint32_t minutes {0};
    uint32_t hours {0};
    uint32_t days {0};
    uint32_t months {0};
    uint32_t topN {0};
    std::vector<uint32_t> range;
    std::vector<std::string> fields;
    std::vector<std::string> titles;
    std::vector<std::string> processes;
    std::vector<std::string> excludeTitles;
    std::vector<std::string> excludeProcesses;
    std::string username;
    bool caseSensitive {false};
};

cxxopts::Options makeOptions();

/**
 * Builds the config from parsed options. When matching is case-insensitive, all
 * include/exclude needles are lowercased.
 */
ReportsConfig makeReportsConfig(const cxxopts::ParseResult& res);

void applyCaseTransform(ReportsConfig& conf);

} // namespace km::reports
