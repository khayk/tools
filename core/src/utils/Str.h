#pragma once

#include <string>

namespace str {

std::wstring s2ws(std::string_view utf8);
std::string ws2s(std::wstring_view utf16);

#if !defined(__cpp_lib_char8_t)
std::string_view u8tos(std::string_view sv);
#else
std::string_view u8tos(std::u8string_view u8sv);
#endif

} // namespace str
