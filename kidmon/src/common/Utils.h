#pragma once

#include <string>
#include <vector>
#include <system_error>

namespace StringUtils
{
    std::wstring s2ws(std::string_view utf8);
    std::string ws2s(std::wstring_view utf16);
}

namespace FileUtils
{
    void write(const std::wstring& filePath, const std::string& data);
    void write(const std::wstring& filePath, const std::vector<char>& data);
}

namespace SysUtils
{
    std::wstring activeUserName();
}

/**
 * @brief Utility functions to provide full paths of know user/system directories
 */
namespace KnownDirs
{
    /**
     * @brief The user home directory. On Windows systems, this is '%USERPROFILE%'.
     */
    std::string home(std::error_code& ec);
    std::string home();

    /**
     * @brief Temp directory of the current account. On Windows systems this is %TEMP%
     */
    std::string temp(std::error_code& ec);
    std::string temp();

    /**
     * @brief Application data directory. On Windows systems, this is '%LOCALAPPDATA%'.
     */
    std::string data(std::error_code& ec);
    std::string data();

    /**
     * @brief Returns the systemwide config directory. On Windows systems, this is '%ALLUSERSPROFILE%'.
     */
    std::string config(std::error_code& ec);
    std::string config();
};