#include "Utils.h"

#ifdef _WIN32
    #include <Shlobj.h>
    #include <Knownfolders.h>
#else
#endif

#include <openssl/sha.h>
#include <fmt/format.h>

#include <system_error>
#include <string_view>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <locale>

namespace fs = std::filesystem;

namespace StringUtils
{
    using ConverterType = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

    std::wstring s2ws(std::string_view utf8)
    {
        return ConverterType().from_bytes(utf8.data(), utf8.data() + utf8.size());
    }

    std::string ws2s(std::wstring_view utf16)
    {
        return ConverterType().to_bytes(utf16.data(), utf16.data() + utf16.size());
    }

} // StringUtils


namespace FileUtils
{
    void write(const std::wstring& filePath, const char* data, size_t size)
    {
        std::ofstream ofs(filePath, std::ios::out | std::ios::binary);

        if (!ofs)
        {
            const auto& s = fmt::format("Unable to open file: {}", StringUtils::ws2s(filePath));
            throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), s);
        }

        ofs.write(data, size);
    }

    void write(const std::wstring& filePath, const std::string& data)
    {
        write(filePath, data.data(), data.size());
    }

    void write(const std::wstring& filePath, const std::vector<char>& data)
    {
        write(filePath, data.data(), data.size());
    }

    std::string fileSha256(const std::wstring& filePath)
    {
        std::ifstream fp(filePath, std::ios::in | std::ios::binary);

        if (!fp.good())
        {
            const auto& s = fmt::format("Unable to open file: {}", StringUtils::ws2s(filePath));
            throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory), s);
        }

        constexpr const std::size_t bufferSize {static_cast<unsigned long long>(1UL) << 12};
        char buffer[bufferSize];

        unsigned char hash[SHA256_DIGEST_LENGTH] = {0};

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        while (fp.good())
        {
            fp.read(buffer, bufferSize);
            SHA256_Update(&ctx, buffer, fp.gcount());
        }

        SHA256_Final(hash, &ctx);
        fp.close();

        std::ostringstream os;
        os << std::hex << std::setfill('0');

        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            os << std::setw(2) << static_cast<unsigned int>(hash[i]);
        }

        return os.str();
    }
} // FileUtils


namespace SysUtils
{
    std::wstring activeUserName()
    {
        return std::wstring();
    }
} // SysUtils 


namespace KnownDirs
{
#ifdef _WIN32
    std::string getKnownFolderPath(const GUID& id, std::error_code& ec) noexcept
    {
        constexpr DWORD flags = KF_FLAG_CREATE;
        HANDLE token = nullptr;
        PWSTR dest = nullptr;

        const HRESULT result = SHGetKnownFolderPath(id, flags, token, &dest);
        std::wstring path;

        if (result != S_OK)
        {
            ec.assign(GetLastError(), std::system_category());
        }
        else
        {
            path.assign(dest);
            CoTaskMemFree(dest);
        }

        return StringUtils::ws2s(path);
    }
#else

#endif
    std::string home(std::error_code& ec)
    {
#ifdef _WIN32
        return getKnownFolderPath(FOLDERID_Profile, ec);
#else
        throw std::logic_error("Not implemented");
#endif
    }

    std::string home()
    {
        std::error_code ec;
        std::string str = home(ec);

        if (ec)
        {
            throw std::system_error(ec, "Failed to retrieve home directory");
        }

        return str;
    }

    std::string temp(std::error_code& ec)
    {
#ifdef _WIN32
        constexpr uint32_t bufferLength = MAX_PATH + 1;
        WCHAR buffer[bufferLength];
        auto length = GetTempPathW(bufferLength, buffer);

        if (length == 0)
        {
            ec.assign(GetLastError(), std::system_category());
        }

        return StringUtils::ws2s(std::wstring_view(buffer, length));
#else
        return std::string();
#endif
    }

    std::string temp()
    {
        std::error_code ec;
        std::string str = temp(ec);

        if (ec)
        {
            throw std::system_error(ec, "Failed to retrieve temp directory");
        }

        return str;
    }

    std::string data(std::error_code& ec)
    {
#ifdef _WIN32
        return getKnownFolderPath(FOLDERID_LocalAppData, ec);
#else
        throw std::logic_error("Not implemented");
#endif
    }

    std::string data()
    {
        std::error_code ec;
        std::string str = data(ec);

        if (ec)
        {
            throw std::system_error(ec, "Failed to retrieve local application data directory");
        }

        return str;
    }

    std::string config(std::error_code& ec)
    {
#ifdef _WIN32
        return getKnownFolderPath(FOLDERID_ProgramData, ec);
#else
        throw std::logic_error("Not implemented");
#endif
    }

    std::string config()
    {
        std::error_code ec;
        std::string str = config(ec);

        if (ec)
        {
            throw std::system_error(ec, "Failed to retrieve program data directory");
        }

        return str;
    }
} // KnownDirs 

