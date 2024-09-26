#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace utl {

void configureLogger(const fs::path& logsDir, const fs::path& logFilename);

} // namespace utl