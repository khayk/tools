#pragma once

#include <kidmon/data/Types.h>

class Filter
{
public:
    explicit Filter(std::string username,
                    TimePoint from = TimePoint::min(),
                    TimePoint to = TimePoint::max())
        : username_ {username}
        , from_ {from}
        , to_ {to}
    {
    }

    const std::string& username() const noexcept
    {
        return username_;
    }

    TimePoint to() const noexcept
    {
        return to_;
    }

    TimePoint from() const noexcept
    {
        return from_;
    }


private:
    std::string username_;
    TimePoint from_;
    TimePoint to_;
};
