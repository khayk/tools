#include <kidmon/common/Utils.h>
#include <core/utils/Crypto.h>

#include <array>
#include <format>
#include <stdexcept>
#include <ctime>

namespace km::utl {

std::string generateToken(const size_t length)
{
    constexpr std::string_view charset = "0123456789"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    // Reject random bytes at or above the largest multiple of the alphabet size
    // that fits in a byte, so every character is drawn uniformly (no modulo
    // bias). For a 62-char alphabet this discards bytes 248..255.
    constexpr unsigned int limit =
        256U - (256U % static_cast<unsigned int>(charset.size()));

    std::string token;
    token.reserve(length);

    // Draw secure random bytes in batches to amortize CSPRNG calls, mapping the
    // accepted ones onto the alphabet until the requested length is reached.
    std::array<unsigned char, 64> buf {};
    while (token.size() < length)
    {
        core::crypto::randomBytes(buf);

        for (const unsigned char byte : buf)
        {
            if (byte < limit)
            {
                token.push_back(charset[byte % charset.size()]);
                if (token.size() == length)
                {
                    break;
                }
            }
        }
    }

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
            std::format("Unable to convert '{}' to local time", dt));
    }

    return d;
}

uint32_t daysSinceYearStart(time_t dt)
{
    return static_cast<uint32_t>(timet2tm(dt).tm_yday);
}

} // namespace km::utl
