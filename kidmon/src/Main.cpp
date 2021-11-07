#include "KidMon.h"
#include "config/Config.h"
#include "common/Console.h"
#include "common/Service.h"
#include "common/Utils.h"
#include "common/Tracer.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

void configureLogger(const Config& cfg)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%L] %v%$");

    auto fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.logsDir + "/kidmon.log");
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%=5t][%L] %v");

    auto logger =
        std::make_shared<spdlog::logger>("multi_sink",
                                         spdlog::sinks_init_list {consoleSink, fileSink});
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(10));
}

int main(int /*argc*/, char* /*argv*/[])
{
    SingleInstanceChecker sic(L"Global\\UniqueName100Percent");

    if (sic.processAlreadyRunning())
    {
        sic.report();
        return 0;
    }
    
    std::unique_ptr<ScopedTrace> trace;

    try
    {
        Config cfg;
        cfg.applyDefaults();
        cfg.applyOverrides(L"");
        configureLogger(cfg);

        trace = std::make_unique<ScopedTrace>("",
                                              fmt::format("{:-^80s}", "> START <"),
                                              fmt::format("{:-^80s}\n", "> END <"));

        ScopedTrace main(__FUNCTION__);
        auto app = std::make_shared<KidMon>(cfg);

        if (sys::isUserInteractive())
        {
            Console console(app);
            console.run();
        }
        else
        {
            Service service(app, "KIDMON");
            service.run();
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }

    return 0;
}
