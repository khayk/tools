#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace core::utl {

fs::path makeLogFilename(std::string_view appName);
void configureLogger(const fs::path& logsDir, const fs::path& logFilename);
void logBuildInfo(std::string_view version,
                  std::string_view commitSha,
                  std::string_view buildTime);

class SilenceLogger
{
    spdlog::level::level_enum prevLevel_ {spdlog::get_level()};

public:
    SilenceLogger();
    ~SilenceLogger();
};

} // namespace core::utl