#include "KidMon.h"
#include "common/Console.h"

#include <spdlog/spdlog.h>

void configureLogger()
{
    spdlog::default_logger()->set_level(spdlog::level::trace);
    spdlog::default_logger()->set_pattern("%^[%L] %v%$");
}

int main(int /*argc*/, char* /*argv*/[])
{
    configureLogger();
    spdlog::info("Working as a console application");

    try
    {
        auto kidMon = std::make_shared<KidMon>();
        Console app(kidMon);

        app.run();
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }

    spdlog::info("Console application is closed");

    return 0;
}
