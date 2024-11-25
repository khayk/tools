#pragma once

#include <string>
#include <ctime>
#include <chrono>

namespace utl {

/**
 * @brief  Generate a alpha-numeric string with a given length
 *
 * @param length The length of the token to be generated
 *
 * @return  The alpha-numeric string
 */
std::string generateToken(size_t length = 16);

/**
 * @brief Get's local time from time_t
 *
 * @param dt The date time
 *
 * @return  The local time as tm structure
 */
tm timet2tm(time_t dt);
bool timet2tm(time_t dt, tm&);

/**
 * @brief  Calculate days since January 1 for the year represented by the given date
 * time
 *
 * @param date  Specified dt
 *
 * @return Number of days
 */
uint32_t daysSinceYearStart(time_t dt = std::time(nullptr));

/**
 * @brief  Converts the value into human friendly text
 *
 * @param value  The duration in milliseconds
 * @param groups  The number of units to show
 *
 * @return  The string representation of the value
 *
 *    1h 3m 4s  ->  1h       (units = 1)
 *    1h 3m 4s  ->  1h 3m    (units = 2)
 *    1h 3m 4s  ->  1h 3m 4s (units = 3)
 */
std::string humanizeDuration(std::chrono::milliseconds value, int units = 2);

} // namespace utl