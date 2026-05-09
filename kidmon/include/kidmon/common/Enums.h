#pragma once

#include <string_view>

enum class ImageFormat : std::uint8_t
{
    jpg,
    bmp,
    gif,
    tif,
    png
};

std::string_view toString(ImageFormat format);
