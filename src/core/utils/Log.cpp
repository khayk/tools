
#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace tools::utl {

fs::path makeLogFilename(std::string_view appName)
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream oss;
    oss << appName << "_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << ".log";

    return oss.str();
}

void configureLogger(const fs::path& logsDir, const fs::path& logFilename)
{
    namespace sinks = spdlog::sinks;
    namespace level = spdlog::level;

    auto consoleSink = std::make_shared<sinks::stdout_color_sink_mt>();
    consoleSink->set_level(level::trace);
    consoleSink->set_pattern("%^[%L] %v%$");

    auto fileSink = std::make_shared<sinks::basic_file_sink_mt>(
        file::path2s(logsDir / logFilename));
    fileSink->set_level(level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%=5t][%L]  %v"); // [%s:%#]

    auto logger = std::make_shared<spdlog::logger>(
        "multi_sink",
        spdlog::sinks_init_list {consoleSink, fileSink});
    logger->set_level(level::trace);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(10));
}

SilenceLogger::SilenceLogger()
{
    spdlog::set_level(spdlog::level::level_enum::off);
}

SilenceLogger::~SilenceLogger()
{
    spdlog::set_level(prevLevel_);
}

} // namespace tools::utl
