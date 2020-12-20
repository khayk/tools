#include "KidMon.h"

#include <spdlog/spdlog.h>

#include <thread>

void KidMon::run()
{
    spdlog::trace("Running KidMon application");

    using namespace std::chrono_literals;

    while (!stopRequested_)
    {
        std::this_thread::sleep_for(10ms);
    }
}

void KidMon::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    stopRequested_ = true;
}
