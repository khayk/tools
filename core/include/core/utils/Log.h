#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>
#include <sstream>
#include <string_view>
#include <vector>

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

// RAII sink that captures spdlog output — useful in unit tests to verify
// that a function emits the expected log messages.
class LogCapture
{
    spdlog::level::level_enum prevLevel_ {spdlog::get_level()};
    std::vector<spdlog::sink_ptr> prevSinks_;
    std::ostringstream oss_;
    spdlog::sink_ptr captureSink_;

public:
    explicit LogCapture(
        spdlog::level::level_enum level = spdlog::level::trace);
    ~LogCapture();

    std::string str() const;
    bool contains(std::string_view text) const;
};

} // namespace core::utl