#pragma once

#include <array>
#include <string>
#include <charconv>

namespace num {

/**
 * @brief Convert str to number. If an error occurred, the value specified in def
 * will be returned.
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

/**
 * @brief  Returns the number of digits of the given number
 *
 * @param number
 * @return
 */
template <typename T>
size_t digits(T number)
{
    size_t digits = 0;

    if constexpr (std::is_signed_v<T>)
    {
        if (number < 0)
        {
            number = -number;
            digits = 1; // remove this line if '-' counts as a digit
        }
    }

    if (number == 0)
    {
        return 1;
    }

    while (number)
    {
        number /= 10;
        ++digits;
    }

    return digits;
}

} // namespace num
