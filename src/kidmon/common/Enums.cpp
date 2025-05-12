#include <kidmon/common/Enums.h>

std::string_view toString(const ImageFormat format)
{
    switch (format)
    {
        case ImageFormat::jpg:
            return "jpg";
        case ImageFormat::bmp:
            return "bmp";
        case ImageFormat::gif:
            return "gif";
        case ImageFormat::tif:
            return "tif";
        case ImageFormat::png:
            return "png";
    }

    return "n/a";
}
