#include "KidMon.h"
#include "config/Config.h"
#include "common/Console.h"

#include <spdlog/spdlog.h>

void configureLogger(const Config& /*cfg*/)
{
    spdlog::default_logger()->set_level(spdlog::level::trace);
    spdlog::default_logger()->set_pattern("%^[%L] %v%$");
}

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
        Config cfg;
        cfg.applyDefaults();
        cfg.applyOverrides(L"");

        configureLogger(cfg);

        spdlog::info("Working as a console application");

        auto kidMon = std::make_shared<KidMon>(cfg);
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
