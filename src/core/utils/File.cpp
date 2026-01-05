#include <core/utils/File.h>
#include <core/utils/Dirs.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Number.h>

#include <boost/iostreams/device/mapped_file.hpp>

#include <fmt/format.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <regex>
#include <stdexcept>

namespace file {

bool open(const fs::path& file,
          const std::ios_base::openmode mode,
          std::ifstream& ifs,
          std::error_code& ec)
{
    ec.clear();
    if (!fs::exists(file, ec))
    {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return false;
    }

    ifs.open(file, mode);

    if (!ifs)
    {
        ec = std::make_error_code(std::errc::permission_denied);
    }

    return !ec;
}

void open(const fs::path& file, const std::ios_base::openmode mode, std::ofstream& ofs)
{
    ofs.open(file, mode);

    if (!ofs)
    {
        const auto s = fmt::format("Unable to open file: {}", file);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }
}


void write(const fs::path& file, const char* const data, size_t size)
{
    std::ofstream ofs;
    open(file, std::ios::out | std::ios::binary, ofs);
    ofs.write(data, static_cast<std::streamsize>(size));
}

void write(const fs::path& file, const std::string_view data)
{
    write(file, data.data(), data.size());
}


void append(const fs::path& file, const char* const data, size_t size)
{
    std::ofstream ofs;
    open(file, std::ios::app | std::ios::binary, ofs);
    ofs.write(data, static_cast<std::streamsize>(size));
}

void append(const fs::path& file, std::string_view data)
{
    append(file, data.data(), data.size());
}


bool read(const fs::path& file, std::string& data, std::error_code& ec)
{
    std::ifstream ifs;
    if (!open(file, std::ios::in | std::ios::binary, ifs, ec))
    {
        return false;
    }

    // Determine the size of the file
    ifs.seekg(0, std::ios::end);
    if (ifs.tellg() < 0)
    {
        ec.assign(static_cast<int>(std::errc::invalid_seek), std::iostream_category());
        return false;
    }

    data.resize(static_cast<size_t>(ifs.tellg()));
    ifs.seekg(0, std::ios::beg);

    // Read the content of the file
    ifs.read(data.data(), static_cast<std::streamsize>(data.size()));

    // Make sure we red all available data
    if (ifs.tellg() != static_cast<std::streamoff>(data.size()))
    {
        ec.assign(static_cast<int>(std::errc::invalid_seek), std::iostream_category());
        return false;
    }

    return true;
}

void readLines(const fs::path& file, const LineCb& cb)
{
    // boost::iostreams::mapped_file throws an exception when attempting to open an
    // empty file in read-only mode
    if (fs::exists(file) && fs::is_empty(file))
    {
        return;
    }

    std::string line;
    boost::iostreams::mapped_file mmap(file.string(),
                                       boost::iostreams::mapped_file::readonly);
    const auto* f = mmap.const_data();
    const auto* e = f + mmap.size();

    while (f && f != e)
    {
        const auto* p =
            static_cast<const char*>(memchr(f, '\n', static_cast<size_t>(e - f)));
        if (p)
        {
            line.assign(f, p);
            f = p + 1;
        }
        else
        {
            line.assign(f, e);
            f = p;
        }

        if (!cb(line))
        {
            return;
        }
    }
}


std::string path2s(const fs::path& path)
{
    return std::string(str::u8tos(path.u8string()));
}


fs::path constructTempPath(std::string_view namePrefix, const fs::path& tempDir)
{
    static std::atomic_uint64_t count = 0;
    fs::path path = tempDir.empty() ? dirs::temp() : tempDir;

    std::string name;

    if (!namePrefix.empty())
    {
        name = namePrefix;
        name.push_back('-');
    }
    else
    {
        name += "tmp-";
    }

    name += num::num2s(sys::currentProcessId());
    name += "-";

    uint64_t n = count.fetch_add(1, std::memory_order_relaxed);

    for (int i = 0; i < 6; ++i)
    {
        name.push_back(static_cast<char>('a' + (n % 26)));
        n /= 26;
    }

    path /= name;
    return path.lexically_normal();
}


Path::Path() noexcept
    : owner_ {false}
{
}


Path::Path(fs::path p) noexcept
    : path_ {std::move(p)}
{
}


const fs::path& Path::path() const noexcept
{
    return path_;
}


bool Path::isOwner() const noexcept
{
    return owner_;
}


void Path::dropOwnership() noexcept
{
    owner_ = false;
}


void Path::takeOwnership(const fs::path& newPath)
{
    path_ = newPath;
    owner_ = true;
}


ScopedDir::ScopedDir(fs::path dirPath)
    : Path {std::move(dirPath)}
{
}

ScopedDir::~ScopedDir()
{
    if (isOwner() && !path().empty())
    {
        std::error_code ec;
        fs::remove_all(path(), ec);
    }
}


TempDir::TempDir(const std::string_view namePrefix,
                 const CreateMode createMode,
                 const fs::path& tempDir)
    : ScopedDir {constructTempPath(namePrefix, tempDir)}
{
    if (createMode == CreateMode::Auto)
    {
        fs::create_directories(path());
    }
}


bool shouldExclude(const fs::path& path, const std::vector<std::regex>& rules)
{
    const std::string pathStr = path.string();

    return std::ranges::any_of(rules, [&pathStr](const auto& rule) {
        return std::regex_search(pathStr, rule);
    });
}


void enumFilesRecursive(const fs::path& dir,
                        const std::vector<std::regex>& exclusionPatterns,
                        const PathCallback& cb)
{
    try
    {
        std::error_code ec {};

        for (const auto& entry : fs::directory_iterator(dir))
        {
            const auto& currentPath = entry.path();

            if (shouldExclude(currentPath, exclusionPatterns) ||
                is_symlink(currentPath))
            {
                continue; // Skip to the next entry
            }

            if (fs::is_directory(currentPath))
            {
                cb(currentPath, ec);
                enumFilesRecursive(currentPath, exclusionPatterns, cb);
            }
            else
            {
                cb(currentPath, ec);
            }
        }
    }
    catch (const std::system_error& error)
    {
        cb(dir, error.code());
    }
    catch (const std::exception&)
    {
        cb(dir, std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

void openDirectory(const fs::path& path)
{
    std::string spath;
    if (fs::is_regular_file(path))
    {
        spath = path2s(path.parent_path());
    }
    else if (fs::is_directory(path))
    {
        spath = path2s(path);
    }
    else
    {
        throw std::runtime_error(
            fmt::format("Unable to navigate directory: {}", path2s(path)));
    }

#ifdef _WIN32
    const std::string command = "explorer \"" + spath + "\"";
#elif __APPLE__
    const std::string command = "open \"" + spath + "\"";
#elif __linux__
    const std::string command = "xdg-open \"" + spath + "\"";
#else
    #error "Unsupported OS"
#endif

    std::ignore = std::system(command.c_str());
}

void navigateFile(const fs::path& file)
{
    const std::string path = path2s(file);

#ifdef _WIN32
    const std::string command = "explorer /select,\"" + path + "\"";
#elif __APPLE__
    const std::string command = "open -R \"" + path + "\"";
#elif __linux__
    constexpr auto pattern =
        R"( dbus-send --session --dest=org.freedesktop.FileManager1 \
        --type=method_call --print-reply \
        /org/freedesktop/FileManager1 \
        org.freedesktop.FileManager1.ShowItems \
        array:string:"file://{}" \
        string:""
    )";
    const std::string command = fmt::format(pattern, path);
#else
    #error "Unsupported OS"
#endif

    std::ignore = std::system(command.c_str());
}

} // namespace file