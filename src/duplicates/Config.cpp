#include <duplicates/Config.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Dirs.h>

#include <spdlog/spdlog.h>
#include <toml++/toml.h>
#include <regex>

using namespace std::literals;
using std::chrono::milliseconds;

namespace tools::dups {
namespace {

std::string concat(const std::vector<std::string>& vec, const std::string& sep)
{
    std::string res;

    for (const auto& item : vec)
    {
        if (!res.empty())
        {
            res.append(sep);
        }
        res.append(item);
    }

    return res;
}

} // namespace

void logConfig(const fs::path& cfgFile, const Config& cfg)
{
    constexpr std::string_view pattern = "{:<27}: '{}'";
    spdlog::trace(pattern, "Configuration file", cfgFile);
    spdlog::trace(pattern, "Log file", cfg.logDir / cfg.logFilename);
    spdlog::trace(pattern, "All files path", cfg.allFilesPath);
    spdlog::trace(pattern, "Duplicate files path", cfg.dupFilesPath);
    spdlog::trace(pattern, "Ignored files path", cfg.ignFilesPath);
    spdlog::trace(pattern, "Scan directories", concat(cfg.scanDirs, ", "));
    spdlog::trace(pattern,
                  "Safe to delete directories",
                  concat(cfg.safeToDeleteDirs, ", "));
    spdlog::trace(pattern, "Cache directory", cfg.cacheDir);
    // spdlog::trace(pattern, "Exclusion patterns", concat(cfg.exclusionPatterns, ",
    // "));
}


Config loadConfig(const fs::path& cfgFile)
{
    Config cfg;
    auto config = toml::parse_file(cfgFile.u8string());

    config["exclusion_patterns"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.exclusionPatterns.emplace_back(std::string(value.value_or(""sv)),
                                               std::regex_constants::ECMAScript);
        }
    });

    config["scan_directories"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.scanDirs.emplace_back(value.value_or(""sv));
        }
    });

    config["safe_to_delete_paths"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.safeToDeleteDirs.emplace_back(value.value_or(""sv));
        }
    });

    constexpr std::string_view appName = "duplicates";
    const fs::path dataDir = dirs::config() / appName;
    const fs::path cacheDir = dirs::cache() / appName;

    cfg.minFileSizeBytes = config["min_file_size_bytes"].value_or(0ULL);
    cfg.maxFileSizeBytes = config["max_file_size_bytes"].value_or(0ULL);
    cfg.updateFrequency = milliseconds(config["update_freq_ms"].value_or(0));
    cfg.allFilesPath = config["all_files"].value_or("");
    cfg.dupFilesPath = config["dup_files"].value_or("");
    cfg.ignFilesPath = config["ign_files"].value_or("");
    cfg.logDir = dataDir / "logs";
    cfg.cacheDir = cacheDir;
    cfg.logFilename = fmt::format("{}.log", appName);

    const auto adjustPath = [&dataDir](fs::path& path) {
        if (!path.empty())
        {
            path = dataDir / path.filename();
        }
    };

    // Ensure that auxilary files are located under the application data directory
    adjustPath(cfg.allFilesPath);
    adjustPath(cfg.dupFilesPath);
    adjustPath(cfg.ignFilesPath);

    return cfg;
}

} // namespace tools::dups
