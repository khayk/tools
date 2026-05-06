#pragma once

#include <system_error>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Utility functions to provide full paths of know user/system directories
 */
namespace dirs {

/**
 * @brief The user home directory. On Windows systems, this is '%USERPROFILE%'.
 */
fs::path home(std::error_code& ec);
fs::path home();


/**
 * @brief Temp directory of the current account. On Windows systems this is %TEMP%
 */
fs::path temp(std::error_code& ec);
fs::path temp();


/**
 * @brief Application data directory. On Windows systems, this is '%LOCALAPPDATA%'.
 */
fs::path data(std::error_code& ec);
fs::path data();


/**
 * @brief Application cache directory.
 */
fs::path cache(std::error_code& ec);
fs::path cache();


/**
 * @brief Returns the systemwide config directory. On Windows systems, this is
 * '%ALLUSERSPROFILE%'.
 */
fs::path config(std::error_code& ec);
fs::path config();

}; // namespace dirs
