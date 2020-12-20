#include "KidMon.h"

#include "common/Singleton.h"
#include "common/Console.h"

#include <spdlog/spdlog.h>

int main(int /*argc*/, char* /*argv*/[])
{
    spdlog::default_logger()->set_level(spdlog::level::trace);
    spdlog::info("Working as a console application");

    try
    {
        auto kidMon = std::make_shared<KidMon>();
        Console app(kidMon);
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }
    
    spdlog::info("Console application is turned off");

    return 0;
}
