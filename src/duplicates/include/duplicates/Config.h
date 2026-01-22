#pragma once

#include <vector>
#include <regex>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace tools::dups {

class Config
{
public:
    Config(fs::path dataDir, fs::path cacheDir);

    const std::vector<fs::path>& scanDirs() const noexcept;
    void setScanDirs(std::vector<fs::path> dirs);
    void addScanDir(fs::path dir);

    const std::vector<fs::path>& dirsToKeepFrom() const noexcept;
    void setDirsToKeepFrom(std::vector<fs::path> dirs);
    void addDirToKeepFrom(fs::path dir);

    const std::vector<fs::path>& dirsToDeleteFrom() const noexcept;
    void setDirsToDeleteFrom(std::vector<fs::path> dirs);
    void addDirToDeleteFrom(fs::path dir);

    const std::vector<std::regex>& exclusionPatterns() const noexcept;
    void setExclusionPatterns(const std::vector<std::string>& patterns);
    void addExclusionPattern(std::string_view pattern);

    const fs::path& dataDir() const noexcept;
    const fs::path& cacheDir() const noexcept;

    const fs::path& allFilesPath() const noexcept;
    void setAllFilesPath(fs::path path);

    const fs::path& dupFilesPath() const noexcept;
    void setDupFilesPath(fs::path path);

    const fs::path& ignFilesPath() const noexcept;
    void setIgnFilesPath(fs::path path);

    const fs::path& keepFilesPath() const noexcept;
    void setKeepFilesPath(fs::path path);

    const fs::path& delFilesPath() const noexcept;
    void setDelFilesPath(fs::path path);

    const fs::path& logDir() const noexcept;
    void setLogDir(fs::path path);

    const fs::path& logFilename() const noexcept;
    void setLogFilename(fs::path filename);

    size_t minFileSizeBytes() const noexcept;
    void setMinFileSizeBytes(size_t bytes);

    size_t maxFileSizeBytes() const noexcept;
    void setMaxFileSizeBytes(size_t bytes);

    std::chrono::milliseconds updateFrequency() const noexcept;
    void setUpdateFrequency(std::chrono::milliseconds freq);

    bool skipDetection() const noexcept;
    void setSkipDetection(bool value);

    bool dryRun() const noexcept;
    void setDryRun(bool value);

private:
    std::vector<fs::path> scanDirs_;
    std::vector<fs::path> dirsToKeepFrom_;
    std::vector<fs::path> dirsToDeleteFrom_;
    std::vector<std::regex> exclusionPatterns_;
    fs::path dataDir_;
    fs::path cacheDir_;
    fs::path allFilesPath_;
    fs::path dupFilesPath_;
    fs::path ignFilesPath_;
    fs::path keepFilesPath_;
    fs::path delFilesPath_;
    fs::path logDir_;
    fs::path logFilename_;
    size_t minFileSizeBytes_ {};
    size_t maxFileSizeBytes_ {};
    std::chrono::milliseconds updateFrequency_ {};
    bool skipDetection_ {false};
    bool dryRun_ {true};
};

void logConfig(const Config& cfg);

void applyDefaults(Config& cfg);
void applyOverrides(const fs::path& cfgFile, Config& cfg);

} // namespace tools::dups
