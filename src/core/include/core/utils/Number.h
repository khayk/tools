#pragma once

#include <array>
#include <string>
#include <charconv>

namespace num {

/**
 * @brief Convert str to number. If an error occurred, the value specified in def will be
 * returned.
 *
 * @param str An input string
 * @param def The default value to be returned in case of any error during conversion
 */
template <typename T>
    requires std::integral<T> || std::floating_point<T>
T s2num(std::string_view str, T def = T())
{
    T value {};
    if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        ec == std::errc())
    {
        return value;
    }

    return def;
}

/**
 * @brief Converts number to string. Returns empty string in case of any error.
 *
 * @param num An input number to be converted to string
 */
template <typename T>
    requires std::integral<T> || std::floating_point<T>
std::string num2s(T num)
{
    std::array<char, std::numeric_limits<T>::digits10 + 2> buf {};

    if (auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), num);
        ec == std::errc())
    {
        return std::string(buf.data(), static_cast<size_t>(ptr - buf.data()));
    }

    return {};
}

} // namespace num
