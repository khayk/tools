#pragma once

#include <duplicates/Config.h>
#include <cxxopts.hpp>

#include <optional>
#include <ostream>

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

/**
 * @brief Run the --list-runs and --restore commands if requested
 *
 * Both work on the runs kept in the cache directory and replace the normal
 * scan/delete pipeline. --restore honours the configured dry-run setting.
 *
 * @param opts Parsed and ready to use command line options
 * @param cfg Populated configuration
 * @param out Stream for the listing, progress and summary
 * @return The process exit code if a command ran (caller should exit):
 *         0 on success, 2 if a restore left conflicts or failures behind.
 *         std::nullopt if no restore command was requested.
 */
std::optional<int> runRestoreCommands(const cxxopts::ParseResult& opts,
                                      const Config& cfg,
                                      std::ostream& out);

} // namespace tools::dups
