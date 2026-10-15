#include <kidmon/repo/RepositoryFactory.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/repo/SqliteRepository.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>

#include <array>
#include <format>
#include <stdexcept>
#include <utility>

namespace km {

namespace {

constexpr std::array PREFIXES = {
    std::pair {std::string_view("fs:"), RepositoryType::FileSystem},
    std::pair {std::string_view("sqlite:"), RepositoryType::Sqlite},
};

std::string_view typeName(RepositoryType type) noexcept
{
    return type == RepositoryType::Sqlite ? "sqlite" : "fs";
}

} // namespace

RepositorySpec parseRepositorySpec(std::string_view spec)
{
    // Only known names count as a prefix, so a drive letter ("C:") never does
    for (const auto& [prefix, type] : PREFIXES)
    {
        if (spec.starts_with(prefix))
        {
            const auto location = spec.substr(prefix.size());
            if (location.empty())
            {
                throw std::invalid_argument(
                    std::format("Repository '{}' has no location", spec));
            }

            return {type, fs::path(location)};
        }
    }

    const fs::path location(spec);

    if (fs::is_directory(location))
    {
        return {RepositoryType::FileSystem, location};
    }

    const auto ext = core::str::asciiLower(core::file::path2s(location.extension()));
    if (ext == ".db" || ext == ".sqlite" || ext == ".sqlite3")
    {
        return {RepositoryType::Sqlite, location};
    }

    throw std::invalid_argument(
        std::format("Can't tell the type of repository '{}', prefix it with 'fs:' "
                    "or 'sqlite:'",
                    spec));
}

std::string toString(const RepositorySpec& spec)
{
    return std::format("{}:{}",
                       typeName(spec.type),
                       core::file::path2s(spec.location));
}

std::unique_ptr<IRepository> openRepository(const RepositorySpec& spec,
                                            RepositoryAccess access)
{
    const bool exists = spec.type == RepositoryType::FileSystem
                            ? fs::is_directory(spec.location)
                            : fs::is_regular_file(spec.location);

    // Opening a missing SQLite file would silently create an empty one
    if (access == RepositoryAccess::Read && !exists)
    {
        throw std::invalid_argument(
            std::format("Repository '{}' does not exist", toString(spec)));
    }

    if (spec.type == RepositoryType::Sqlite)
    {
        return std::make_unique<SqliteRepository>(spec.location);
    }

    return std::make_unique<FileSystemRepository>(spec.location);
}

} // namespace km
