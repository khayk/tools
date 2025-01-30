#include <core/utils/Dirs.h>
#include <kidmon/config/Config.h>

using namespace std::chrono_literals;

AppConfig::AppConfig()
    : logFilename("kidmon.log")
{
    appDataDir = dirs::data().append("kidmon").lexically_normal();
    logsDir = appDataDir / "logs";
}
