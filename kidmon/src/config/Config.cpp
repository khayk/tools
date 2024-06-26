#include <kidmon/config/Config.h>
#include <kidmon/common/Utils.h>

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

    serverPort = 1234;
}

void Config::applyOverrides(const fs::path& filename)
{
    snapshotInterval = 10s;
    logFilename = filename;
}