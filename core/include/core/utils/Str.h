#pragma once

#include <string>

namespace str {

std::wstring s2ws(std::string_view utf8);
void s2ws(std::string_view utf8, std::wstring& wstr);

std::string ws2s(std::wstring_view utf16);
void ws2s(std::wstring_view utf16, std::string& utf8);

#if !defined(__cpp_lib_char8_t)
std::string_view u8tos(std::string_view sv);
std::string_view stou8(std::string_view sv);
#else
std::string_view u8tos(std::u8string_view u8sv);
std::u8string_view stou8(std::string_view sv);
#endif

} // namespace str
