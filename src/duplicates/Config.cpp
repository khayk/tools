#include <duplicates/Config.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Dirs.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <toml++/toml.h>
#include <chrono>
#include <filesystem>
#include <regex>
#include <algorithm>

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

std::string concat(const std::vector<fs::path>& vec, const std::string& sep)
{
    std::vector<std::string> strVec;
    strVec.reserve(vec.size());

    std::ranges::transform(vec, std::back_inserter(strVec), [](const fs::path& p) {
        return file::path2s(p);
    });
    return concat(strVec, sep);
}

// Ensure that auxilary files are located under the application data directory
void adjustPath(const fs::path& dataDir, fs::path& path) {
    if (!path.empty() && !path.is_absolute())
    {
        path = dataDir / path.filename();
    }
};

} // namespace

Config::Config(fs::path dataDir, fs::path cacheDir)
    : dataDir_(std::move(dataDir))
    , cacheDir_(std::move(cacheDir))
{
    constexpr std::string_view appName = "duplicates";

    dataDir_     = dirs::config() / appName;
    cacheDir_    = dirs::cache() / appName;
    logDir_      = dataDir_ / "logs";
    logFilename_ = tools::utl::makeLogFilename(appName);
}

const std::vector<fs::path>& Config::scanDirs() const noexcept {
    return scanDirs_;
}

void Config::setScanDirs(std::vector<fs::path> dirs) {
    scanDirs_ = std::move(dirs);
}

void Config::addScanDir(fs::path dir) {
    scanDirs_.push_back(std::move(dir));
}

const std::vector<fs::path>& Config::preferredDeletionDirs() const noexcept {
    return safeDeletionDirs_;
}

void Config::setPreferredDeletionDirs(std::vector<fs::path> dirs) {
    safeDeletionDirs_ = std::move(dirs);
}

void Config::addPreferredDeletionDir(fs::path dir) {
    safeDeletionDirs_.push_back(std::move(dir));
}

const std::vector<std::regex>& Config::exclusionPatterns() const noexcept {
    return exclusionPatterns_;
}

void Config::setExclusionPatterns(const std::vector<std::string>& patterns) {
    exclusionPatterns_.clear();
    for (const auto& pattern: patterns) {
        addExclusionPattern(pattern);
    }
}

void Config::addExclusionPattern(std::string_view pattern) {
    exclusionPatterns_.emplace_back(
        std::string(pattern), std::regex_constants::ECMAScript
    );
}

const fs::path& Config::dataDir() const noexcept {
    return dataDir_;
}

const fs::path& Config::cacheDir() const noexcept {
    return cacheDir_;
}

const fs::path& Config::allFilesPath() const noexcept {
    return allFilesPath_;
}

void Config::setAllFilesPath(fs::path path) {
    allFilesPath_ = std::move(path);
    adjustPath(dataDir(), allFilesPath_);
}

const fs::path& Config::dupFilesPath() const noexcept {
    return dupFilesPath_;
}

void Config::setDupFilesPath(fs::path path) {
    dupFilesPath_ = std::move(path);
    adjustPath(dataDir(), dupFilesPath_);
}

const fs::path& Config::ignFilesPath() const noexcept {
    return ignFilesPath_;
}

void Config::setIgnFilesPath(fs::path path) {
    ignFilesPath_ = std::move(path);
    adjustPath(dataDir(), ignFilesPath_);
}

const fs::path& Config::logDir() const noexcept {
    return logDir_;
}

void Config::setLogDir(fs::path path) {
    logDir_ = std::move(path);
}

const fs::path& Config::logFilename() const noexcept {
    return logFilename_;
}

void Config::setLogFilename(fs::path filename) {
    logFilename_ = std::move(filename);
}

size_t Config::minFileSizeBytes() const noexcept {
    return minFileSizeBytes_;
}

void Config::setMinFileSizeBytes(size_t bytes) {
    minFileSizeBytes_ = bytes;
}

size_t Config::maxFileSizeBytes() const noexcept {
    return maxFileSizeBytes_;
}

void Config::setMaxFileSizeBytes(size_t bytes) {
    maxFileSizeBytes_ = bytes;
}

std::chrono::milliseconds Config::updateFrequency() const noexcept {
    return updateFrequency_;
}

void Config::setUpdateFrequency(std::chrono::milliseconds freq) {
    updateFrequency_ = freq;
}

bool Config::skipDetection() const noexcept {
    return skipDetection_;
}

void Config::setSkipDetection(bool value) {
    skipDetection_ = value;
}

bool Config::dryRun() const noexcept {
    return dryRun_;
}

void Config::setDryRun(bool value) {
    dryRun_ = value;
}

// --------------------------------------------------------------------------------------
//                                 HELPER FUNCTIONS
// --------------------------------------------------------------------------------------

void applyDefaults(Config& cfg)
{
    cfg.setMinFileSizeBytes(1024);
    cfg.setMaxFileSizeBytes(10UL * 1024 * 1024 * 1024);
    cfg.setUpdateFrequency(std::chrono::milliseconds(100));
    cfg.setAllFilesPath("all.txt");
    cfg.setIgnFilesPath("ignored.txt");
    cfg.setDupFilesPath("duplicates.txt");
}

void logConfig(const Config& cfg)
{
    constexpr std::string_view pattern = "{:<27}: '{}'";

    spdlog::trace(pattern, "Log file", cfg.logDir() / cfg.logFilename());
    spdlog::trace(pattern, "All files path", cfg.allFilesPath());
    spdlog::trace(pattern, "Duplicate files path", cfg.dupFilesPath());
    spdlog::trace(pattern, "Ignored files path", cfg.ignFilesPath());
    spdlog::trace(pattern, "Scan directories", concat(cfg.scanDirs(), ", "));
    spdlog::trace(pattern, "Safe to delete directories", concat(cfg.preferredDeletionDirs(), ", "));
    spdlog::trace(pattern, "Cache directory", cfg.cacheDir());
    spdlog::trace(pattern, "Min file size bytes", cfg.minFileSizeBytes());
    spdlog::trace(pattern, "Max file size bytes", cfg.maxFileSizeBytes());
    spdlog::trace(pattern, "Dry run", cfg.dryRun());
    // spdlog::trace(pattern, "Exclusion patterns", concat(cfg.exclusionPatterns, ",
    // "));
}

void applyOverrides(const fs::path& cfgFile, Config& cfg)
{
    if (cfgFile.empty()) {
        return;
    }

    if (!fs::exists(cfgFile)) {
        spdlog::warn("Missing config file: {}", cfgFile);
        return;
    }

    spdlog::info("Overriding config from file: {}", cfgFile);

    auto config = toml::parse_file(cfgFile.string());

    config["exclusion_patterns"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.addExclusionPattern(std::string(value.value_or(""sv)));
        }
    });

    config["scan_directories"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.addScanDir(value.value_or(""sv));
        }
    });

    config["preferred_deletion_dirs"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.addPreferredDeletionDir(value.value_or(""sv));
        }
    });

    cfg.setMinFileSizeBytes(config["min_file_size_bytes"].value_or(cfg.minFileSizeBytes()));
    cfg.setMaxFileSizeBytes(config["max_file_size_bytes"].value_or(cfg.maxFileSizeBytes()));
    cfg.setUpdateFrequency(milliseconds(config["update_freq_ms"].value_or(cfg.updateFrequency().count())));

    if (config.contains("all_files"))
    {
        cfg.setAllFilesPath(config["all_files"].value_or(""));
    }

    if (config.contains("dup_files"))
    {
        cfg.setDupFilesPath(config["dup_files"].value_or(""));
    }

    if (config.contains("ign_files"))
    {
        cfg.setIgnFilesPath(config["ign_files"].value_or(""));
    }

    cfg.setDryRun(config["dry_run"].value_or(cfg.dryRun()));

}

} // namespace tools::dups
