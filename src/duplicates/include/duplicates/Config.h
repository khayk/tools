#pragma once

#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace tools::dups {

struct Config
{
    std::vector<std::string> scanDirs;
    std::vector<std::string> safeToDeleteDirs;
    std::vector<std::regex> exclusionPatterns;
    fs::path cacheDir;
    fs::path allFilesPath;
    fs::path dupFilesPath;
    fs::path ignFilesPath;
    fs::path logDir;
    fs::path logFilename;
    size_t minFileSizeBytes {};
    size_t maxFileSizeBytes {};
    std::chrono::milliseconds updateFrequency {};
    bool skipDetection {false};
};


Config loadConfig(const fs::path& cfgFile);
void logConfig(const fs::path& cfgFile, const Config& cfg);

} // namespace tools::dups
