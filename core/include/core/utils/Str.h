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

/**
 * @brief Removes whitespace from the beginning of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trimLeft(std::string& s);

/**
 * @brief Removes whitespace from the end of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trimRight(std::string& s);

/**
 * @brief Removes whitespace from both ends of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trim(std::string& s);

} // namespace str
