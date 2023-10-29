#include "Str.h"

#include <codecvt>

namespace str {
using ConverterType = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

std::wstring s2ws(std::string_view utf8)
{
    return ConverterType().from_bytes(utf8.data(), utf8.data() + utf8.size());
}

std::string ws2s(std::wstring_view utf16)
{
    return ConverterType().to_bytes(utf16.data(), utf16.data() + utf16.size());
}

#if !defined(__cpp_lib_char8_t)
std::string u8tos(const std::string& s)
{
    return s;
}

std::string u8tos(std::string&& s)
{
    return std::move(s);
}
#else
std::string u8tos(const std::u8string_view& s)
{
    return std::string(s.begin(), s.end());
}
#endif

} // namespace str
