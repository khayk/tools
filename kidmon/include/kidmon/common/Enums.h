#pragma once

#include <string>

enum class ImageFormat
{
    jpg,
    bmp,
    gif,
    tif,
    png
};

std::string_view toString(ImageFormat format);
