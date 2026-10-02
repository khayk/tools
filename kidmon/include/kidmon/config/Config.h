#pragma once

#include <spdlog/common.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace km {

struct AppConfig
{
    AppConfig();

    fs::path appDataDir;
    fs::path logsDir;
    fs::path logFilename;
    spdlog::level::level_enum logLevel {spdlog::level::trace};
};

} // namespace km
