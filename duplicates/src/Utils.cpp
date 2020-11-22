#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "Utils.h"
#include <codecvt>

using ConverterType = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

std::wstring s2ws(const std::string& utf8)
{
    return ConverterType().from_bytes(utf8);
}

std::string ws2s(const std::wstring& utf16)
{
    return ConverterType().to_bytes(utf16);
}

uint16_t digits(size_t n)
{
    uint16_t d = 0;

    while (n != 0)
    {
        d++;
        n /= 10;
    }

    return d;
}
