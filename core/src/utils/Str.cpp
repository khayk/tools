#include "Str.h"

#include <codecvt>
#include <locale>

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
std::string_view u8tos(const std::string_view sv)
{
    return sv;
}
#else
std::string_view u8tos(const std::u8string_view u8sv)
{
    return std::string_view(reinterpret_cast<const char*>(u8sv.data()), u8sv.size());
}
#endif

} // namespace str
