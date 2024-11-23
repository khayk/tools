#pragma once

#include <filesystem>

namespace fs = std::filesystem;

struct AppConfig
{
    AppConfig();

    fs::path appDataDir;
    fs::path logsDir;
    fs::path logFilename;
};
