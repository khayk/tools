#include <pwd.h>
#include <unistd.h>
#include <cstdlib>

#include <core/utils/Dirs.h>

namespace core::dirs {

fs::path home(std::error_code& ec)
{
    if (const char* dir = std::getenv("HOME"))
    {
        return fs::path {dir};
    }

    // $HOME is not set (e.g. system daemon launched by launchd/systemd).
    // Fall back to the passwd database which always has the home directory.
    if (const passwd* pw = getpwuid(getuid()); pw && pw->pw_dir)
    {
        return fs::path {pw->pw_dir};
    }

    ec.assign(static_cast<int>(std::errc::invalid_argument), std::system_category());
    return {};
}

fs::path temp(std::error_code& ec)
{
    return fs::temp_directory_path(ec);
}

fs::path data(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir.append(".data");
    }
    return dir;
}

fs::path cache(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir.append(".cache");
    }
    return dir;
}

fs::path config(std::error_code& ec)
{
    fs::path dir = home(ec);
    if (!ec)
    {
        dir.append(".config");
    }
    return dir;
}

} // namespace core::dirs
