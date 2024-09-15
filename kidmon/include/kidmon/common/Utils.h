#pragma once

#include <string>
#include <ctime>

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
 * @brief  Calculate days since January 1 for the year represented by the given date time
 *
 * @param date  Specified dt
 *
 * @return Number of days
 */
uint32_t daysSinceYearStart(time_t dt = std::time(nullptr));

} // namespace utl