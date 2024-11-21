#pragma once

#include <core/utils/File.h>
#include <fmt/format.h>

namespace fs = std::filesystem;

template <>
class fmt::formatter<fs::path>
{
public:
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }
    template <typename Context>
    constexpr auto format(fs::path const& ph, Context& ctx) const
    {
        return format_to(ctx.out(), "{}", file::path2s(ph));
    }
};

