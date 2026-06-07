#pragma once

#include <string_view>

namespace km::detail {

// Raw entry files are named "raw-DDD-MMDD.dat" where DDD is the zero-padded
// day-of-year. Within a single year that prefix makes the file names sort
// chronologically, so a plain lexicographic comparison against the boundary
// file names is enough to decide whether a file *could* hold entries inside the
// queried [from, to] window. The per-entry timestamp check at read time stays
// the source of truth; the helpers below are purely a "can this file/year be
// skipped wholesale" optimization.
//
// A year argument of 0 means that side of the range is unbounded (the bounding
// timestamp could not be converted to a calendar year). An empty boundary file
// name likewise means that side is unbounded.

// Whether `year`'s directory can hold any entry within the queried range.
inline bool yearInRange(int year, int yearFrom, int yearTo) noexcept
{
    if (yearFrom != 0 && year < yearFrom)
    {
        return false;
    }

    if (yearTo != 0 && year > yearTo)
    {
        return false;
    }

    return true;
}

// Whether the raw file `fn`, living in `year`'s directory, can hold any entry
// within the queried range. The lower bound only constrains files in the first
// year of the range (yearFrom); the upper bound only constrains files in the
// last year (yearTo). In any year strictly between the two every file is in
// range. This is what makes the year boundaries correct: a December file in the
// start year must not be rejected by the end year's day-of-year cutoff, and a
// January file in the end year must not be rejected by the start year's cutoff.
inline bool rawFileInRange(std::string_view fn,
                           int year,
                           int yearFrom,
                           std::string_view fnFrom,
                           int yearTo,
                           std::string_view fnTo) noexcept
{
    if (year == yearFrom && !fnFrom.empty() && fn < fnFrom)
    {
        return false;
    }

    if (year == yearTo && !fnTo.empty() && fn > fnTo)
    {
        return false;
    }

    return true;
}

} // namespace km::detail
