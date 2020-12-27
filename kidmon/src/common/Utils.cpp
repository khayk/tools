#include "Utils.h"

#include <fmt/format.h>

#include <string_view>
#include <fstream>
#include <codecvt>
#include <locale>
#include <system_error>

namespace StringUtils
{
    using ConverterType = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

    std::wstring s2ws(std::string_view utf8)
    {
        return ConverterType().from_bytes(utf8.data(), utf8.data() + utf8.size());
    }

    std::string ws2s(std::wstring_view utf16)
    {
        return ConverterType().to_bytes(utf16.data(), utf16.data() + utf16.size());
    }

} // StringUtils

namespace FileUtils
{
    void write(const std::wstring& filePath, const char* data, size_t size)
    {
        std::ofstream ofs(filePath, std::ios::out | std::ios::binary);

        if (!ofs)
        {
            const auto& s = fmt::format("Unable to open file: {}", StringUtils::ws2s(filePath));
            throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), s);
        }

        ofs.write(data, size);
    }

    void write(const std::wstring& filePath, const std::string& data)
    {
        write(filePath, data.data(), data.size());
    }

    void write(const std::wstring& filePath, const std::vector<char>& data)
    {
        write(filePath, data.data(), data.size());
    }

} // FileUtils

