#pragma once

#include <string>
#include <vector>
#include <system_error>
#include <filesystem>

namespace fs = std::filesystem;

namespace str {

std::wstring s2ws(std::string_view utf8);
std::string ws2s(std::wstring_view utf16);

#if !defined(__cpp_lib_char8_t)
std::string u8tos(const std::string& s);
std::string u8tos(std::string&& s);
#else
std::string u8tos(const std::u8string_view& s);
#endif

} // namespace str

namespace file {

void write(const fs::path& file, const std::string& data);
void write(const fs::path& file, const std::vector<char>& data);

std::string path2s(const fs::path& path);

} // namespace file

namespace crypto {

std::string fileSha256(const fs::path& file);

} // crypto

namespace sys {

std::wstring activeUserName();

bool isUserInteractive() noexcept;

std::string errorDescription(uint64_t code);

void logError(std::string_view message, uint64_t errorCode) noexcept;

void logLastError(const std::string_view message) noexcept;

} // namespace sys

/**
 * @brief Utility functions to provide full paths of know user/system directories
 */
namespace dirs
{
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
     * @brief Returns the systemwide config directory. On Windows systems, this is '%ALLUSERSPROFILE%'.
     */
    fs::path config(std::error_code& ec);
    fs::path config();
};

class SingleInstanceChecker
{
    std::wstring appName_;
    std::atomic_bool processAlreadyRunning_ {false};
    void* mutex_ {nullptr};

public:
    SingleInstanceChecker(std::wstring_view name);
    ~SingleInstanceChecker();

    bool processAlreadyRunning() const noexcept;
    void report() const;
};

