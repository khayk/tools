#include <kidmon/common/Utils.h>
#include <core/utils/Str.h>

#include <spdlog/spdlog.h>
#include <ctime>

namespace fs = std::filesystem;


namespace utl {

std::string generateToken(const size_t length)
{
    const auto randomChar = []() -> char {
        constexpr std::string_view charset = "0123456789"
                                             "abcdefghijklmnopqrstuvwxyz"
                                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        return charset[static_cast<unsigned long>(rand()) % (charset.size() - 1)];
    };

    std::string token(length, 0);
    std::generate_n(token.begin(), length, randomChar);

    return token;
}

bool timet2tm(time_t dt, tm& d)
{
#ifdef _WIN32
    if (localtime_s(&d, &dt))
    {
        return false;
    }
#else
    if (!localtime_r(&dt, &d) || errno == EOVERFLOW)
    {
        return false;
    }
#endif

    return d.tm_year >= 0 && d.tm_year <= 300;
}

tm timet2tm(const time_t dt)
{
    tm d {};

    if (!timet2tm(dt, d))
    {
        throw std::runtime_error(
            fmt::format("Unable to convert '{}' to local time", dt));
    }

    return d;
}

uint32_t daysSinceYearStart(time_t dt)
{
    return static_cast<uint32_t>(timet2tm(dt).tm_yday);
}

} // namespace utl