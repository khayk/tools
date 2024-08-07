#pragma once

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
    std::chrono::milliseconds snapshotInterval {5000};
    std::chrono::milliseconds peerDropTimeout {activityCheckInterval.count() + 2000};
    bool takeSnapshots {false};

    uint16_t serverPort {1234};
    bool spawnAgent {true};
    bool calcSha {false};
    std::string authToken;

    void applyDefaults();
    void applyOverrides(const fs::path& filename);
};
