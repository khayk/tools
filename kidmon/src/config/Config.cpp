#include <core/utils/Dirs.h>
#include <kidmon/config/Config.h>

using namespace std::chrono_literals;

namespace km {

AppConfig::AppConfig()
    : logFilename("kidmon.log")
{
    appDataDir = core::dirs::data().append("kidmon").lexically_normal();
    logsDir = appDataDir / "logs";
}

} // namespace km
