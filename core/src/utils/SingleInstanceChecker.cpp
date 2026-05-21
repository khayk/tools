#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Str.h>
#include <spdlog/spdlog.h>

namespace core {

bool SingleInstanceChecker::processAlreadyRunning() const noexcept
{
    return processAlreadyRunning_;
}

void SingleInstanceChecker::report() const
{
    spdlog::info("One instance of '{}' is already running.", str::ws2s(appName_));
}

} // namespace core
