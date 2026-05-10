#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace km {

struct AppConfig
{
    AppConfig();

    fs::path appDataDir;
    fs::path logsDir;
    fs::path logFilename;
};

} // namespace km
