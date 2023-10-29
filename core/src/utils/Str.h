#pragma once

#include <string>

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
