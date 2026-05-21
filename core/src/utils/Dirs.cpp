#include <core/utils/Dirs.h>

namespace core::dirs {

fs::path home()
{
    std::error_code ec;
    auto path = home(ec);
    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve home directory");
    }
    return path;
}

fs::path temp()
{
    std::error_code ec;
    auto path = temp(ec);
    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve temp directory");
    }
    return path;
}

fs::path data()
{
    std::error_code ec;
    auto path = data(ec);
    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve application data directory");
    }
    return path;
}

fs::path cache()
{
    std::error_code ec;
    auto path = cache(ec);
    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve cache directory");
    }
    return path;
}

fs::path config()
{
    std::error_code ec;
    auto path = config(ec);
    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve program data directory");
    }
    return path;
}

} // namespace core::dirs
