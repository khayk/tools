#pragma once

#include "Utils.h"
#include <fmt/format.h>

namespace fs = std::filesystem;

template <>
struct fmt::formatter<fs::path> // NOSONAR cpp:S1642 "struct" names should comply with a
                                // naming convention
{
    template <typename ParseContext> // NOSONAR cpp:S6024 Free functions should be
                                     // preferred to member functions when accessing a
                                     // container in a generic context
    constexpr auto parse(ParseContext& ctx) // NOSONAR cpp:S4334 "auto" should not
                                            // be used to deduce raw pointers
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const fs::path& ph, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{0}", file::path2s(ph));
    }
};
