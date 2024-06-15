#pragma once

#include <cstdint>

class Dimensions
{
    uint32_t width_ {0};
    uint32_t height_ {0};

public:
    Dimensions() noexcept = default;
    Dimensions(uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
    {
    }

    // clang-format off
    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    void width(uint32_t value) noexcept { width_ = value; }
    void height(uint32_t value) noexcept { height_ = value; }
    // clang-format on
};