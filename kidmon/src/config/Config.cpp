#include "Config.h"
#include "common/Utils.h"

using namespace std::chrono_literals;

void Config::applyDefaults()
{
    appDataDir = dirs::data().append("kidmon").lexically_normal();
    reportsDir = appDataDir / "reports";
    logsDir = appDataDir / "logs";
    logFilename = "kidmon.log";

    activityCheckInterval = 5s;
    snapshotInterval = 5min;

    serverPort = 1234;
}

void Config::applyOverrides(const fs::path& file)
{
    snapshotInterval = 3s;
    logFilename = file;
}