#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace core::file::detail {

/**
 * @brief Builds the OS command to open the directory containing @p path.
 *        If @p path is a regular file the parent directory is used.
 *
 * @throws std::runtime_error when @p path is neither a regular file nor a directory.
 */
std::string buildOpenDirCommand(const fs::path& path);

/**
 * @brief Builds the OS command to open the file manager and select @p file.
 */
std::string buildNavigateFileCommand(const fs::path& file);

} // namespace core::file::detail
