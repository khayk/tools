#pragma once

#include <string_view>

enum class ImageFormat
{
    jpg,
    bmp,
    gif,
    tif,
    png
};

std::string_view toString(ImageFormat format);
