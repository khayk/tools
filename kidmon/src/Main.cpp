#include "KidmonAgent.h"
#include "KidmonServer.h"
#include "config/Config.h"
#include "common/Console.h"
#include "common/Service.h"
#include "common/Utils.h"
#include "common/Tracer.h"

#include <utils/File.h>
#include <utils/Str.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <optional>

void configureLogger(const Config& cfg)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%L] %v%$");

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        file::path2s(cfg.logsDir / cfg.logFilename));
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%=5t][%L] %v");

    auto logger =
        std::make_shared<spdlog::logger>("multi_sink",
                                         spdlog::sinks_init_list {consoleSink, fileSink});
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(10));
}

int main(int argc, char* argv[])
{
    const bool agentMode = argc > 1;
    std::wstring uniqueName = L"KidmonUniqueName";
    fs::path logFile = "kidmon";

    if (agentMode)
    {
        uniqueName.append(L"Agent_" + sys::activeUserName());
        logFile.concat("-agent");
    }

    logFile.concat(".log");

    SingleInstanceChecker sic(uniqueName);

    if (sic.processAlreadyRunning())
    {
        sic.report();

        return 1;
    }

    std::optional<ScopedTrace> trace;

    try
    {
        Config cfg;

        cfg.applyDefaults();
        cfg.applyOverrides(logFile);
        configureLogger(cfg);
        cfg.authToken = argc > 1 ? argv[1] : "";

        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));

        ScopedTrace main(__FUNCTION__);

        spdlog::trace("Active username: {}", str::ws2s(sys::activeUserName()));
        spdlog::info("Unique guard string: {}", str::ws2s(uniqueName));

        std::shared_ptr<Runnable> app;
        const bool isInteractive = sys::isUserInteractive();

        if (agentMode)
        {
            app = std::make_shared<KidmonAgent>(cfg);
        }
        else
        {
            app = std::make_shared<KidmonServer>(cfg);
        }

        if (isInteractive)
        {
            Console console(app);
            console.run();
        }
        else
        {
            Service service(app, "KIDMON");
            service.run();
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }

    return 2;
}
