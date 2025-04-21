#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/server/KidmonServer.h>
#include <kidmon/config/Config.h>
#include <kidmon/common/Console.h>
#include <kidmon/common/Service.h>
#include <kidmon/common/Utils.h>
#include <BuildInfo.h>

#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/Log.h>
#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Tracer.h>

#include <cxxopts.hpp>

#include <iostream>
#include <optional>

namespace {

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

    std::time_t t = std::time(0); // get time now
    std::tm now = utl::timet2tm(t);
    const auto date = fmt::format("-{}-{:02}-{:02}",
                                  now.tm_year + 1900,
                                  now.tm_mon + 1,
                                  now.tm_mday);

    logFile.concat(date);
    logFile.concat(".log");
}

} // namespace

int main(int argc, char* argv[])
{
    cxxopts::Options opts("kidmon", "Monitor kid activity on a PC");
    std::optional<ScopedTrace> trace;
    std::optional<ScopedTrace> traceMain;

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
        const std::string token = result["token"].as<std::string>();

        if (result.count("help"))
        {
            std::cout << opts.help() << '\n';
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

        AppConfig appConf;
        appConf.logFilename = logFile;

        // Configure logger as soon as possible
        utl::configureLogger(appConf.logsDir, appConf.logFilename);

        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
        traceMain.emplace(__FUNCTION__);

        spdlog::info("Build time: {}", BuildInfo::Timestamp);
        spdlog::info("Commit SHA: {}", BuildInfo::CommitSHA);
        spdlog::info("Version: {}", BuildInfo::Version);
        spdlog::debug("Active username: {}", str::ws2s(sys::activeUserName()));

        std::shared_ptr<Runnable> app;
        const bool isInteractive = sys::isUserInteractive();

        if (agentMode)
        {
            KidmonAgent::Config conf;
            conf.authToken = token;

            app = std::make_shared<KidmonAgent>(conf);
        }
        else
        {
            KidmonServer::Config conf(appConf.appDataDir);
            conf.authToken = token;
            conf.spawnAgent = !result["passive"].as<bool>();

            app = std::make_shared<KidmonServer>(conf);
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
