#pragma once

#include <cxxopts.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace km::reports {

struct ReportsConfig
{
    uint32_t minutes {0};
    uint32_t hours {0};
    uint32_t days {0};
    uint32_t months {0};
    std::vector<uint32_t> top;
    std::vector<uint32_t> range;
    std::vector<std::string> titles;
    std::vector<std::string> processes;
    std::vector<std::string> excludeTitles;
    std::vector<std::string> excludeProcesses;
    std::string username;
    bool caseSensitive {false};
};

cxxopts::Options makeOptions();

/**
 * Finds options that can't be used together: --list with --migrate-to, or either of
 * them with query options, which they would silently ignore. Returns the error
 * message, if any.
 */
std::optional<std::string> findOptionConflict(const cxxopts::ParseResult& res);

/**
 * Builds the config from parsed options. When matching is case-insensitive, all
 * include/exclude needles are lowercased.
 */
ReportsConfig makeReportsConfig(const cxxopts::ParseResult& res);

void applyCaseTransform(ReportsConfig& conf);

} // namespace km::reports
