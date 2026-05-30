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

/**
 * @brief Run the hidden benchmark/metrics review if requested
 *
 * When the hidden --metrics-file option is supplied, loads the listed paths
 * into a detector and logs resource usage, then signals the caller to exit.
 * This is a debug-only feature kept out of the normal scan/delete pipeline.
 *
 * @param opts Parsed and ready to use command line options
 * @return true if the metrics review ran (caller should exit), false otherwise
 */
bool runMetricsReview(const cxxopts::ParseResult& opts);

} // namespace tools::dups
