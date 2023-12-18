#pragma once

#include "Constants.h"
#include <string>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

// @todo:hayk - make server and agent have their own config
struct Config
{
    fs::path appDataDir;
    fs::path reportsDir;
    fs::path logsDir;
    fs::path logFilename;

    std::chrono::milliseconds activityCheckInterval {5000};
    std::chrono::milliseconds snapshotInterval {0};

    uint16_t serverPort {1234};
    std::string authToken;

    void applyDefaults();
    void applyOverrides(const fs::path& /*file*/);
};
