#pragma once

#include <system_error>
#include <filesystem>
#include <atomic>

namespace fs = std::filesystem;


namespace sys {

std::wstring activeUserName();

bool isUserInteractive() noexcept;

std::string errorDescription(uint64_t code);

std::string constructErrorMsg(std::string_view message, uint64_t errorCode);

std::string constructLastErrorMsg(std::string_view message);

void logError(std::string_view message, uint64_t errorCode);

void logLastError(std::string_view message);

/**
 * @brief Full path of the current process
 *
 * @return Current process path with executable name.
 */
fs::path currentProcessPath();

} // namespace sys

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
 * @brief Returns the systemwide config directory. On Windows systems, this is
 * '%ALLUSERSPROFILE%'.
 */
fs::path config(std::error_code& ec);
fs::path config();

}; // namespace dirs


class SingleInstanceChecker
{
    std::wstring appName_;
    std::atomic_bool processAlreadyRunning_ {false};
    void* mutex_ {nullptr};

public:
    explicit SingleInstanceChecker(std::wstring_view name);
    ~SingleInstanceChecker();

    bool processAlreadyRunning() const noexcept;
    void report() const;
};

namespace utl {

/**
 * @brief  Generate a alpha-numeric string with a given length
 *
 * @param length The length of the token to be generated
 *
 * @return  The alpha-numeric string
 */
std::string generateToken(size_t length = 16);

/**
 * @brief Get's local time from time_t
 *
 * @param dt The date time
 *
 * @return  The local time as tm structure
 */
tm timet2tm(time_t dt);
bool timet2tm(time_t dt, tm&);

/**
 * @brief  Calculate days since January 1 for the year represented by the given date time
 *
 * @param date  Specified dt
 *
 * @return Number of days
 */
uint32_t daysSinceYearStart(time_t dt = std::time(nullptr));

} // namespace utl