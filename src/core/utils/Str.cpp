#include <core/utils/Str.h>
#include <cassert>
#include <clocale>
#include <algorithm>
#include <cwctype>
#include <ranges>
#include <fmt/format.h>

namespace str {

namespace {

class LocaleInitializer
{
    const char* prevLocale_ {std::setlocale(LC_ALL, "en_US.utf8")};

public:
    LocaleInitializer()
    {
        if (prevLocale_ == nullptr)
        {
            prevLocale_ = std::setlocale(LC_ALL, "C.utf8");
        }

        if (prevLocale_ == nullptr)
        {
            std::puts("WARNING: Failed to set local, utf8 conversions wan't work");
        }
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
    mbsrtowcs_s(&len, wstr.data(), len, &mbstr, len, &state);
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
    s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) noexcept {
                return std::isgraph(ch);
            }));

    return s;
}

std::string& trimRight(std::string& s)
{
    s.erase(std::ranges::find_if(std::ranges::reverse_view(s),
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

std::string_view trimLeft(std::string_view sv)
{
    while (!sv.empty() && !std::isgraph(static_cast<unsigned char>(sv.front())))
    {
        sv.remove_prefix(1);
    }

    return sv;
}

std::string_view trimRight(std::string_view sv)
{
    while (!sv.empty() && !std::isgraph(static_cast<unsigned char>(sv.back())))
    {
        sv.remove_suffix(1);
    }

    return sv;
}

std::string_view trim(std::string_view sv)
{
    return trimLeft(trimRight(sv));
}

std::string& asciiLowerInplace(std::string& str)
{
    std::ranges::transform(str, str.begin(), [](const unsigned char c) noexcept {
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
    std::ranges::transform(str, str.begin(), [](const auto& c) noexcept {
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


std::string humanizeDuration(std::chrono::milliseconds ms, int units)
{
    using namespace std::chrono;
    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);
    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);
    auto hour = duration_cast<hours>(mins);
    mins -= duration_cast<minutes>(hour);
    auto day = duration_cast<days>(hour);
    hour -= duration_cast<hours>(day);

    std::ostringstream oss;

    if (day.count() > 0 && units > 0)
    {
        --units;
        oss << day.count() << "d ";
    }

    if (hour.count() > 0 && units > 0)
    {
        --units;
        oss << hour.count() << "h ";
    }

    if (mins.count() > 0 && units > 0)
    {
        --units;
        oss << mins.count() << "m ";
    }

    if (secs.count() > 0 && units > 0)
    {
        --units;
        oss << secs.count() << "s ";
    }

    if (ms.count() > 0 && units > 0)
    {
        oss << ms.count() << "ms ";
    }

    std::string out = oss.str();
    if (!out.empty() && out.back() == ' ')
    {
        out.pop_back();
    }

    return out;
}


std::string humanizeBytes(size_t bytes)
{
    const char* const units[] = {"B", "Kb", "Mb", "Gb", "Tb", "Pb"};
    int i = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && i < 5)
    {
        size /= 1024;
        i++;
    }

    return fmt::format("{:.2f} {}", size, units[i]);
}


} // namespace str
