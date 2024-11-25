
#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace utl {

void configureLogger(const fs::path& logsDir, const fs::path& logFilename)
{
    namespace sinks = spdlog::sinks;
    namespace level = spdlog::level;

    auto consoleSink = std::make_shared<sinks::stdout_color_sink_mt>();
    consoleSink->set_level(level::trace);
    consoleSink->set_pattern("%^[%L] %v%$");

    auto fileSink =
        std::make_shared<sinks::basic_file_sink_mt>(file::path2s(logsDir / logFilename));
    fileSink->set_level(level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%=5t][%L]  %v "); // [%s:%#]

    auto logger =
        std::make_shared<spdlog::logger>("multi_sink",
                                         spdlog::sinks_init_list {consoleSink, fileSink});
    logger->set_level(level::trace);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(10));
}

} // namespace utl
