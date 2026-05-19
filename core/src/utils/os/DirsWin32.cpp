#include <Shlobj.h>
#include <Knownfolders.h>

#include <core/utils/Dirs.h>

namespace {

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

} // namespace

namespace core::dirs {

fs::path home(std::error_code& ec)
{
    return getKnownFolderPath(FOLDERID_Profile, ec);
}

fs::path temp(std::error_code& ec)
{
    constexpr uint32_t bufferLength = MAX_PATH + 1;
    WCHAR buffer[bufferLength];
    const auto length = GetTempPathW(bufferLength, buffer);

    if (length == 0)
    {
        ec.assign(static_cast<int>(GetLastError()), std::system_category());
        return {};
    }

    return fs::path(std::wstring_view(buffer, length));
}

fs::path data(std::error_code& ec)
{
    return getKnownFolderPath(FOLDERID_LocalAppData, ec);
}

fs::path cache(std::error_code& ec)
{
    return getKnownFolderPath(FOLDERID_LocalAppData, ec);
}

fs::path config(std::error_code& ec)
{
    return getKnownFolderPath(FOLDERID_ProgramData, ec);
}

} // namespace core::dirs
