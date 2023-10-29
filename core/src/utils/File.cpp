#include "File.h"
#include "Str.h"
#include "FmtExt.h"

#include <fmt/format.h>
#include <fstream>

namespace file {

void write(const fs::path& file, const char* const data, size_t size)

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

void write(const fs::path& file, const std::string_view data)
{
    write(file, data.data(), data.size());
}

bool read(const fs::path& file, std::string& data, std::error_code& ec)
{
    std::ifstream ifs {file, std::ios::in | std::ios::binary};
    ec.clear();

    if (!ifs)
    {
        ec.assign(static_cast<int>(std::errc::no_such_file_or_directory),
                  std::iostream_category());
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


std::string path2s(const fs::path& path)
{
    return str::u8tos(path.u8string());
}

} // namespace file