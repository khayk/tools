#include <core/utils/Dirs.h>
#include <kidmon/config/Config.h>

using namespace std::chrono_literals;

void Config::applyDefaults()
{
    appDataDir = dirs::data().append("kidmon").lexically_normal();
    reportsDir = appDataDir / "reports";
    logsDir = appDataDir / "logs";
    logFilename = "kidmon.log";

    activityCheckInterval = 2s;
    peerDropTimeout = activityCheckInterval + 2s;
    snapshotInterval = 5min;

    serverPort = 51097;
}

void Config::applyOverrides(const fs::path& filename)
{
    snapshotInterval = 10s;
    logFilename = filename;
}