#include <core/utils/Str.h>

#include <codecvt>
#include <locale>

namespace str {

namespace {

class LocaleInitializer
{
    const char* prevLocale_ {nullptr};

public:
    LocaleInitializer()
    {
        prevLocale_ = std::setlocale(LC_ALL, "en_US.utf8");
    }
};

LocaleInitializer localeInitializer;

} // namespace

void s2ws(std::string_view utf8, std::wstring& wstr)
{
    const char* mbstr = utf8.data();
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = std::mbsrtowcs(nullptr, &mbstr, 0, &state);
    wstr.resize(len);
    std::mbsrtowcs(&wstr[0], &mbstr, wstr.size(), &state);
}

std::wstring s2ws(std::string_view utf8)
{
    std::wstring wstr;
    s2ws(utf8, wstr);

    return wstr;
}

void ws2s(std::wstring_view utf16, std::string& utf8)
{
    const wchar_t* wstr = utf16.data();
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = std::wcsrtombs(nullptr, &wstr, 0, &state);
    utf8.resize(len);
    std::wcsrtombs(&utf8[0], &wstr, utf8.size(), &state);
}

std::string ws2s(std::wstring_view utf16)
{
    std::string utf8;
    ws2s(utf16, utf8);

    return utf8;
}

#if !defined(__cpp_lib_char8_t)
std::string_view u8tos(const std::string_view sv)
{
    return sv;
}

std::string_view stou8(const std::string_view sv)
{
    return sw;
}

#else
std::string_view u8tos(const std::u8string_view u8sv)
{
    return {reinterpret_cast<const char*>(u8sv.data()), u8sv.size()};
}

std::u8string_view stou8(std::string_view sv)
{
    return {reinterpret_cast<const char8_t*>(sv.data()), sv.size()};
}

#endif

} // namespace str
