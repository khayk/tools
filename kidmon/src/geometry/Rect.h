#pragma once

#include "Point.h"
#include "Dimensions.h"

class Rect
{
    Point leftTop_;
    Dimensions dims_;

public:
    Rect() noexcept = default;
    Rect(Point leftTop, Dimensions dims) noexcept
        : leftTop_(leftTop)
        , dims_(dims)
    {
    }

    // clang-format off
    uint32_t width() const noexcept { return dims_.width(); }
    uint32_t height() const noexcept { return dims_.height(); }

    Point leftTop() const noexcept { return leftTop_; }
    Point leftBottom() const noexcept { return leftTop_ + Point(0, height()); }
    Point rightTop() const noexcept { return leftTop_ + Point(width(), 0); }
    Point rightBottom() const noexcept { return leftTop_ + Point(width(), height()); }
    // clang-format on
};