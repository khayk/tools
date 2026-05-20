#include <core/utils/Dirs.h>

namespace core::dirs {

fs::path data(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= "Library/Application Support";
    }
    return dir;
}

fs::path cache(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= "Library/Caches";
    }
    return dir;
}

fs::path config(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= "Library/Preferences";
    }
    return dir;
}

} // namespace core::dirs
