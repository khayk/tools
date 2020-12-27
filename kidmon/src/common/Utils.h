#pragma once

#include <string>
#include <vector>

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
