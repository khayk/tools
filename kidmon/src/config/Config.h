#pragma once

#include "Constants.h"
#include <string>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

struct Config
{
    fs::path appDataDir;
    fs::path reportsDir;
    fs::path logsDir;
    fs::path logFilename;

    std::chrono::milliseconds activityCheckInterval {5000};
    std::chrono::milliseconds snapshotInterval {0};

    void applyDefaults();
    void applyOverrides(const fs::path& /*file*/);
};
