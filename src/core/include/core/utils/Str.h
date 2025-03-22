#pragma once

#include <string>
#include <chrono>

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
 * @brief Removes whitespaces and any control characters from the beginning of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trimLeft(std::string& s);
std::string_view trimLeft(std::string_view sv);

/**
 * @brief Removes whitespaces and any control characters from the end of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trimRight(std::string& s);
std::string_view trimRight(std::string_view sv);

/**
 * @brief Removes whitespaces and any control characters from both ends of a string
 *
 * @param s string to be trimmed
 *
 * @return trimmed string
 */
std::string& trim(std::string& s);
std::string_view trim(std::string_view sv);


/**
 * @brief Converts given string to lowercase, by modifying input string
 *
 * @param str Input string to be converted to lowercase
 *
 * @return  Reference the to input string, in lowercase
 */
std::string& asciiLowerInplace(std::string& str);

/**
 * @brief Converts given string to lowercase, by modifying input string
 *
 * @param str Input string to be converted to lowercase
 * @param buf Input wstring, useful to improve performance. If provided,
 *            the underlying implementation will use it to convert string to
 *            wide string before doing lowercase transformation
 *
 * @return  Reference the to input string, in lowercase
 */
std::string& utf8LowerInplace(std::string& str, std::wstring* buf = 0);


/**
 * @brief Converts given wstring to lowercase, by modifying input string
 *
 * @param wstr Input wstring to be converted to lowercase
 *
 * @return  Reference the to input wstring, in lowercase
 */
std::wstring& lowerInplace(std::wstring& wstr);


/**
 * @brief Converts input string to lowercase producing a new string
 *
 * @param str An input string to be converted to lowercase
 *
 * @return  A new string object, in lowercase
 */
std::string asciiLower(const std::string& str);


/**
 * @brief Converts input string to lowercase producing a new string
 *
 * @param str An input string to be converted to lowercase
 * @param buf Input wstring, reusing same buffer can result to improved performance.
 *            When provided, the underlying implementation will use it to convert string
 *            to wide string before doing lowercase transformation
 *
 * @return  A new string object, in lowercase
 */
std::string utf8Lower(const std::string& str, std::wstring* buf = 0);

/**
 * @brief  Converts the value into human friendly text
 *
 * @param value  The duration in milliseconds
 * @param units  The number of units to show
 *
 * @return  The string representation of the value
 *
 *    1h 3m 4s  ->  1h       (units = 1)
 *    1h 3m 4s  ->  1h 3m    (units = 2)
 *    1h 3m 4s  ->  1h 3m 4s (units = 3)
 */
std::string humanizeDuration(std::chrono::milliseconds ms, int units = 2);

/**
 * @brief  Constructs a human friendly representation of bytes
 *
 * @param bytes  Number of bytes
 *
 * @return  The string representation of the given number of bytes
 */
std::string humanizeBytes(size_t bytes);

} // namespace str
