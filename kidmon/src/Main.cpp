#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/server/KidmonServer.h>
#include <kidmon/config/Config.h>
#include <kidmon/common/Console.h>
#include <kidmon/common/Service.h>
#include <kidmon/common/Utils.h>
#include <kidmon/common/Tracer.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <cxxopts.hpp>

#include <iostream>
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

void constructAttribs(const bool agent, std::wstring& uniqueName, fs::path& logFile)
{
    uniqueName = L"kmuid";
    logFile = "kidmon";

    if (agent)
    {
        uniqueName.append(L"-agent-" + sys::activeUserName());
        logFile.concat("-agent");
    }
    else
    {
        uniqueName.append(L"-server");
        logFile.concat("-server");
    }

    logFile.concat(".log");
}

int main(int argc, char* argv[])
{
    cxxopts::Options opts("kidmon", "Monitor kid activity on a PC");
    std::optional<ScopedTrace> trace;

    // clang-format off
    opts.add_options()
        ("t,token", "Authorization token for agent", cxxopts::value<std::string>()->default_value(""))
        ("a,agent", "Run as an agent", cxxopts::value<bool>()->default_value("false"))
        ("p,passive", "Run server in a passive mode", cxxopts::value<bool>()->default_value("false"));

    // clang-format on
    try
    {
        auto result = opts.parse(argc, argv);
        const bool agentMode = result["agent"].as<bool>();

        if (result.count("help"))
        {
            std::cout << opts.help() << std::endl;
            return 0;
        }

        std::wstring uniqueName;
        fs::path logFile;
        constructAttribs(agentMode, uniqueName, logFile);
        SingleInstanceChecker sic(uniqueName);

        if (sic.processAlreadyRunning())
        {
            sic.report();
            return 1;
        }

        Config cfg;

        cfg.applyDefaults();
        cfg.applyOverrides(logFile);
        configureLogger(cfg);
        cfg.authToken = result["token"].as<std::string>();
        cfg.spawnAgent = !result["passive"].as<bool>();

        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
        ScopedTrace main(__FUNCTION__);
        spdlog::trace("Active username: {}", str::ws2s(sys::activeUserName()));

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
