#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/server/KidmonServer.h>
#include <kidmon/config/Config.h>
#include <kidmon/data/Constants.h>
#include <core/app/Console.h>
#include <core/app/Service.h>
#include <kidmon/common/Utils.h>
#include <BuildInfo.h>

#include <core/utils/Str.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Sys.h>
#include <core/utils/Log.h>
#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Tracer.h>
#include <spdlog/spdlog.h>

#include <cxxopts.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>

using namespace km;
using core::Console;
using core::Runnable;
using core::Service;

namespace {

void constructAttribs(const bool agent, std::wstring& uniqueName, fs::path& logFile)
{
    uniqueName = L"kmuid";
    logFile = "kidmon";

    if (agent)
    {
        uniqueName.append(L"-agent-" + core::sys::activeUserName());
        logFile.concat("-agent");
    }
    else
    {
        uniqueName.append(L"-server");
        logFile.concat("-server");
    }

    std::time_t t = std::time(nullptr);
    std::tm now = utl::timet2tm(t);
    const auto date = std::format("-{}-{:02}-{:02}",
                                  now.tm_year + 1900,
                                  now.tm_mon + 1,
                                  now.tm_mday);

    logFile.concat(date);
    logFile.concat(".log");
}

// Read an environment variable and immediately remove it from the process
// environment so the secret does not linger in this process (e.g. readable via
// /proc/self/environ). Returns an empty string if the variable is unset.
std::string readAndClearEnv(const char* name)
{
    std::string value;

#ifdef _WIN32
    // std::getenv triggers C4996 on MSVC; _dupenv_s is the recommended,
    // bounds-checked replacement (allocates a copy the caller must free).
    char* v = nullptr;
    size_t len = 0;
    if (_dupenv_s(&v, &len, name) == 0 && v != nullptr)
    {
        value = v;
    }
    free(v);

    _putenv_s(name, "");
#else
    if (const char* v = std::getenv(name))
    {
        value = v;
    }

    ::unsetenv(name);
#endif

    return value;
}

} // namespace

int main(int argc, char* argv[])
{
    std::optional<ScopedTrace> trace;
    std::optional<ScopedTrace> traceMain;

    try
    {
        cxxopts::Options opts("kidmon", "Monitor kid activity on a PC");

        // clang-format off
        opts.add_options()
            ("t,token", "Authorization token for agent", cxxopts::value<std::string>()->default_value(""))
            ("a,agent", "Run as an agent", cxxopts::value<bool>()->default_value("false"))
            ("p,passive", "Run server in a passive mode", cxxopts::value<bool>()->default_value("false"))
            ("h,help", "Print usage");

        // clang-format on

        auto result = opts.parse(argc, argv);
        const bool agentMode = result["agent"].as<bool>();
        const std::string token = result["token"].as<std::string>();

        if (result.contains("help"))
        {
            std::cout << opts.help() << '\n';
            return 0;
        }

        std::wstring uniqueName;
        fs::path logFile;
        constructAttribs(agentMode, uniqueName, logFile);
        core::SingleInstanceChecker sic(uniqueName);

        if (sic.processAlreadyRunning())
        {
            sic.report();
            return 1;
        }

        AppConfig appConf;
        appConf.logFilename = logFile;

        // Configure logger as soon as possible
        core::utl::configureLogger(appConf.logsDir, appConf.logFilename);

        trace.emplace("",
                      std::format("{:-^80s}", "> START <"),
                      std::format("{:-^80s}\n", "> END <"));
        traceMain.emplace(__FUNCTION__);
        core::utl::logBuildInfo(BuildInfo::Version,
                                BuildInfo::CommitSHA,
                                BuildInfo::Timestamp);
        spdlog::trace("Logs file: {}", appConf.logsDir / appConf.logFilename);
        spdlog::debug("Active username: {}",
                      core::str::ws2s(core::sys::activeUserName()));

        std::shared_ptr<Runnable> app;
        const bool isInteractive = core::sys::isUserInteractive();

        if (agentMode)
        {
            KidmonAgent::Config conf;
            conf.authToken = token;

            // The server delivers the token out of band via the environment so
            // it is not exposed on the agent's command line. An explicit
            // --token (manual runs) takes precedence when provided.
            if (conf.authToken.empty())
            {
                conf.authToken =
                    readAndClearEnv(std::string(constants::ENV_AUTH_TOKEN).c_str());
            }

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
