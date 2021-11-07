#include "Config.h"
#include "common/Utils.h"

#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

void Config::applyDefaults()
{
    appDataDir =
        fs::path(KnownDirs::data()).append("kidmon").lexically_normal().u8string();

    reportsDir = fs::path(appDataDir).append("reports").u8string();
    logsDir = fs::path(appDataDir).append("logs").u8string();

    activityCheckIntervalMs = 5 * SECOND_MS;
    snapshotIntervalMs = 5 * MINUTE_MS;
}

void Config::applyOverrides(const std::wstring& /*filePath*/)
{
    snapshotIntervalMs = 3 * SECOND_MS;
}