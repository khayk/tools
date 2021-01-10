#include "KidMon.h"
#include "config/Config.h"
#include "common/Console.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

void configureLogger(const Config& cfg)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%L] %v%$");
    
    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.logsDir + "/kidmon.log");
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%L] %v");

    auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list {consoleSink, fileSink});
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
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
