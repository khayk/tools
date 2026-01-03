#pragma once

#include <duplicates/Config.h>
#include <cxxopts.hpp>

namespace tools::dups {

/**
 * @brief Define valid set of program options
 *
 * @param opts The options to be altered
 */
void defineOptions(cxxopts::Options& opts);

/**
 * @brief Initialize configuration based on command line arguments
 *
 * @param opts Parsed and ready to use command line options
 * @param cfg The configuration to be populated
 */
void populateConfig(const cxxopts::ParseResult& opts, Config& cfg);

} // namespace tools::dups
