#include <core/utils/Str.h>
#include <cassert>
#include <clocale>
#include <algorithm>
#include <wctype.h>

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
    size_t len = 0;

#ifdef _WIN32
    mbsrtowcs_s(&len, nullptr, 0, &mbstr, 0, &state);
    wstr.resize(len > 0 ? len - 1 : 0);
    mbsrtowcs_s(&len, &wstr[0], len, &mbstr, len, &state);
#else
    len = 1 + std::mbsrtowcs(nullptr, &mbstr, 0, &state);
    wstr.resize(len > 0 ? len - 1 : 0);
    std::mbsrtowcs(&wstr[0], &mbstr, wstr.size(), &state);
#endif
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
    std::size_t len = 0;

#ifdef _WIN32
    auto err = wcsrtombs_s(&len, nullptr, 0, &wstr, 0, &state);
    assert(err == 0);
    utf8.resize(len > 0 ? len - 1 : 0);
    err = wcsrtombs_s(&len, utf8.data(), len, &wstr, len, &state);
    assert(err == 0);
#else
    len = std::wcsrtombs(nullptr, &wstr, 0, &state);
    utf8.resize(len);
    std::wcsrtombs(&utf8[0], &wstr, utf8.size(), &state);
#endif
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

std::string& trimLeft(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](const unsigned char ch) noexcept {
                return std::isgraph(ch);
            }));

    return s;
}

std::string& trimRight(std::string& s)
{
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         [](const unsigned char ch) noexcept {
                             return std::isgraph(ch);
                         })
                .base(),
            s.end());

    return s;
}

std::string& trim(std::string& s)
{
    return trimLeft(trimRight(s));
}

std::string& asciiLowerInplace(std::string& str)
{
    std::transform(str.begin(),
                   str.end(),
                   str.begin(),
                   [](const unsigned char c) noexcept {
                       return static_cast<char>(std::tolower(c));
                   });

    return str;
}

std::string& utf8LowerInplace(std::string& str, std::wstring* buf)
{
    std::wstring wstr;
    buf = (buf ? buf : &wstr);
    s2ws(str, *buf);
    lowerInplace(*buf);
    ws2s(*buf, str);

    return str;
}

std::wstring& lowerInplace(std::wstring& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](const auto& c) noexcept {
        return static_cast<std::wstring::value_type>(towlower(static_cast<wint_t>(c)));
    });
    return str;
}

std::string asciiLower(const std::string& str)
{
    std::string tmp = str;
    asciiLowerInplace(tmp);

    return tmp;
}

std::string utf8Lower(const std::string& str, std::wstring* buf)
{
    std::string tmp = str;
    utf8LowerInplace(tmp, buf);

    return tmp;
}


} // namespace str
