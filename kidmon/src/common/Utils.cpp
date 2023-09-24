#include "Utils.h"
#include "FmtExt.h"
#ifdef _WIN32
    #include <Shlobj.h>
    #include <Knownfolders.h>
#else
#endif

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>

#include <system_error>
#include <string_view>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <array>

namespace fs = std::filesystem;

namespace str {
using ConverterType = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

std::wstring s2ws(std::string_view utf8)
{
    return ConverterType().from_bytes(utf8.data(), utf8.data() + utf8.size());
}

std::string ws2s(std::wstring_view utf16)
{
    return ConverterType().to_bytes(utf16.data(), utf16.data() + utf16.size());
}

#if !defined(__cpp_lib_char8_t)
std::string u8tos(const std::string& s)
{
    return s;
}

std::string u8tos(std::string&& s)
{
    return std::move(s);
}
#else
std::string u8tos(const std::u8string_view& s)
{
    return std::string(s.begin(), s.end());
}
#endif

} // namespace str


namespace file {

void write(const fs::path& file, const char* data, size_t size)
{
    std::ofstream ofs(file, std::ios::out | std::ios::binary);

    if (!ofs)
    {
        const auto s = fmt::format("Unable to open file: {}", file);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    ofs.write(data, size);
}

void write(const fs::path& file, const std::string& data)
{
    write(file, data.data(), data.size());
}

void write(const fs::path& file, const std::vector<char>& data)
{
    write(file, data.data(), data.size());
}

std::string path2s(const fs::path& path)
{
    return str::u8tos(path.u8string());
}

} // namespace file

namespace crypto {

std::string fileSha256(const fs::path& file)
{
    std::ifstream in(file, std::ios::in | std::ios::binary);

    if (!in.good())
    {
        const auto s = fmt::format("Unable to open file: {}", file);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    constexpr const std::size_t bufferSize {static_cast<unsigned long long>(1UL) << 12};
    char buffer[bufferSize];
    unsigned char hash[EVP_MAX_MD_SIZE] = {0};

    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX* ctx)> ctx(EVP_MD_CTX_new(),
                                                               EVP_MD_CTX_free);

    const EVP_MD* md = EVP_get_digestbyname("sha256");
    EVP_DigestInit_ex(ctx.get(), md, nullptr);
    
    while (in)
    {
        in.read(buffer, bufferSize);
        if (!EVP_DigestUpdate(ctx.get(), buffer, in.gcount()))
        {
            throw std::runtime_error("Digest update failed");
        }
    }

    uint32_t mdLen = 0;
    EVP_DigestFinal_ex(ctx.get(), hash, &mdLen);
    in.close();

    std::string out;
    out.resize(mdLen + mdLen);

    for (uint32_t i = 0; i < mdLen; ++i)
    {
        sprintf(out.data() + 2 * i, "%02x", hash[i]);
    }

    return out;
}


} // namespace crypto


namespace sys {

std::wstring activeUserName()
{
    // @todo:khayk
    return std::wstring();
}

bool isUserInteractive() noexcept
{
    bool interactiveUser = true;

    HWINSTA hWinStation = GetProcessWindowStation();

    if (hWinStation != nullptr)
    {
        USEROBJECTFLAGS uof = {0};
        if (GetUserObjectInformation(hWinStation,
                                     UOI_FLAGS,
                                     &uof,
                                     sizeof(USEROBJECTFLAGS),
                                     NULL) &&
            ((uof.dwFlags & WSF_VISIBLE) == 0))
        {
            interactiveUser = false;
        }
    }

    return interactiveUser;
}

std::string errorDescription(uint64_t code)
{
    std::string message;

#ifdef _WIN32
    if (code == 0)
    {
        return message; /// No error message has been recorded
    }

    std::array<CHAR, 512> buffer {};

    const size_t size =
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       static_cast<DWORD>(code),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buffer.data(),
                       static_cast<DWORD>(buffer.size()),
                       nullptr);

    if (size > 0)
    {
        /// get rid of \r\n
        message.assign(buffer.data(), std::min<size_t>(size, size - 2));
    }

    return message;
#else
    message = std::strerror(code);
    return message;
#endif
}

std::string constructMessage(const std::string_view message, const uint64_t errorCode)
{
    if (errorCode == 0)
    {
        return std::string(message);
    }

    return fmt::format("{}, errorCode = {}, desc: {}",
                       message,
                       errorCode,
                       errorDescription(errorCode));
}

void logError(const std::string_view message, const uint64_t errorCode) noexcept
{
    try
    {
        const std::string output = constructMessage(message, errorCode);

        // We should not report these errors by default
        spdlog::error(output);
    }
    catch (const std::exception& ex)
    {
        std::ignore = ex;
    }
}

void logLastError(const std::string_view message) noexcept
{
    logError(message, GetLastError());
}

} // namespace sys


namespace dirs {

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
        ec.assign(GetLastError(), std::system_category());
    }
    else
    {
        path = dest;
        CoTaskMemFree(dest);
    }

    return path;
}
#else

#endif
fs::path home(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_Profile, ec);
#else
    throw std::logic_error("Not implemented");
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
        ec.assign(GetLastError(), std::system_category());
    }
    else
    {
        path.assign(std::wstring_view(buffer, length));
    }
#else
    throw std::logic_error("Not implemented");
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
    throw std::logic_error("Not implemented");
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
    throw std::logic_error("Not implemented");
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

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
    mutex_ = CreateMutexW(nullptr, TRUE, appName_.data());
    const auto error = GetLastError();
    
    if (mutex_ == nullptr)
    {
        spdlog::error("CreateMutex failed, ec: {}", error);
        processAlreadyRunning_ = true;

        return;
    }

    processAlreadyRunning_ = (error == ERROR_ALREADY_EXISTS);
}

SingleInstanceChecker::~SingleInstanceChecker()
{
    if (mutex_)
    {
        ReleaseMutex(mutex_);
        CloseHandle(mutex_);
        mutex_ = nullptr;
    }
}

bool SingleInstanceChecker::processAlreadyRunning() const noexcept
{
    return processAlreadyRunning_;
}

void SingleInstanceChecker::report() const
{
    spdlog::info("One instance of '{}' is already running.", str::ws2s(appName_));
}
