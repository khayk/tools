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


/**
 * @brief Converts given string to lowercase, by modifying input string
 *
 * @param str Input string to be converted to lowercase
 *
 * @return  Reference the to input string, in lowercase
 */
std::string& lowerInplace(std::string& str);


/**
 * @brief Converts given wstring to lowercase, by modifying input string
 *
 * @param str Input wstring to be converted to lowercase
 *
 * @return  Reference the to input wstring, in lowercase
 */
std::wstring& lowerInplace(std::wstring& str);


/**
 * @brief Converts input string to lowercase producing a new string
 *
 * @param input An input string to be converted to lowercase
 *
 * @return  A new string object, in lowercase
 */
std::string lower(std::string input);

} // namespace str
