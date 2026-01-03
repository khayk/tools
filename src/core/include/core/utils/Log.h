#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace tools::utl {

fs::path makeLogFilename(std::string_view appName);
void configureLogger(const fs::path& logsDir, const fs::path& logFilename);

class SilenceLogger
{
    spdlog::level::level_enum prevLevel_ {spdlog::get_level()};

public:
    SilenceLogger();
    ~SilenceLogger();
};

} // namespace tools::utl