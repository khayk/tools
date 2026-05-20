#include <core/utils/Dirs.h>

namespace core::dirs {

fs::path data(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= ".local/share";
    }
    return dir;
}

fs::path cache(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= ".cache";
    }
    return dir;
}

fs::path config(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir /= ".config";
    }
    return dir;
}

} // namespace core::dirs
