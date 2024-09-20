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
    [[nodiscard]] uint32_t width() const noexcept { return dims_.width(); }
    [[nodiscard]] uint32_t height() const noexcept { return dims_.height(); }

    [[nodiscard]] Point leftTop() const noexcept { return leftTop_; }
    [[nodiscard]] Point leftBottom() const noexcept {
        return leftTop_ + Point(0, static_cast<int>(height()));
    }
    [[nodiscard]] Point rightTop() const noexcept {
        return leftTop_ + Point(static_cast<int>(width()), 0);
    }
    [[nodiscard]] Point rightBottom() const noexcept {
        return leftTop_ + Point(static_cast<int>(width()), static_cast<int>(height()));
    }
    // clang-format on

    bool operator==(const Rect&) const = default;
};