#ifdef _WIN32
    #include <Shlobj.h>
    #include <Knownfolders.h>
#else
    #include <cstdlib>
    #include <fmt/format.h>
#endif

#include <core/utils/Dirs.h>

namespace {

#ifdef _WIN32

fs::path getKnownFolderPath(const GUID& id, std::error_code& ec) noexcept
{
    constexpr DWORD flags = KF_FLAG_CREATE;
    HANDLE token = nullptr;
    PWSTR dest = nullptr;

    const HRESULT result = SHGetKnownFolderPath(id, flags, token, &dest);
    fs::path path;

    if (result != S_OK)
    {
        ec.assign(static_cast<int>(GetLastError()), std::system_category());
    }
    else
    {
        path = dest;
        CoTaskMemFree(dest);
    }

    return path;
}
#endif

} // namespace


namespace dirs {

fs::path home(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_Profile, ec);
#else
    const char* homeDir = std::getenv("HOME");
    if (!homeDir)
    {
        ec.assign(static_cast<int>(std::errc::invalid_argument),
                  std::system_category());
        return {};
    }

    return fs::path{homeDir};
#endif
}

fs::path home()
{
    std::error_code ec;
    auto str = home(ec);

    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve home directory");
    }

    return str;
}

fs::path temp(std::error_code& ec)
{
    fs::path path;

#ifdef _WIN32
    constexpr uint32_t bufferLength = MAX_PATH + 1;
    WCHAR buffer[bufferLength];
    auto length = GetTempPathW(bufferLength, buffer);

    if (length == 0)
    {
        ec.assign(static_cast<int>(GetLastError()), std::system_category());
    }
    else
    {
        path.assign(std::wstring_view(buffer, length));
    }
#else
    return std::filesystem::temp_directory_path(ec);
#endif

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

fs::path data(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_LocalAppData, ec);
#else
    fs::path dir = home(ec);

    if (!ec)
    {
        dir.append(".data");
    }

    return dir;
#endif
}

fs::path data()
{
    std::error_code ec;
    auto path = data(ec);

    if (ec)
    {
        throw std::system_error(ec,
                                "Failed to retrieve local application data directory");
    }

    return path;
}

fs::path config(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_ProgramData, ec);
#else
    fs::path dir = home(ec);

    if (!ec)
    {
        dir.append(".config");
    }

    return dir;
#endif
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

} // namespace dirs
