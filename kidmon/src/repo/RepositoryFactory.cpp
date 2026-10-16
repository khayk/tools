#include <kidmon/repo/RepositoryFactory.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/repo/SqliteRepository.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>

#include <algorithm>
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

// Whether `target` is below the `source` folder, comparing resolved paths
bool isInside(const RepositorySpec& source, const RepositorySpec& target)
{
    const auto parent = fs::weakly_canonical(source.location);
    const auto child = fs::weakly_canonical(target.location);
    const auto [parentEnd, childIt] = std::ranges::mismatch(parent, child);

    return parentEnd == parent.end() && childIt != child.end();
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
        // A folder repository creates its directories, so match that here
        if (access == RepositoryAccess::Write && spec.location.has_parent_path())
        {
            fs::create_directories(spec.location.parent_path());
        }

        const auto mode = access == RepositoryAccess::Read
                              ? SqliteRepository::Mode::ReadOnly
                              : SqliteRepository::Mode::ReadWrite;
        return std::make_unique<SqliteRepository>(spec.location, mode);
    }

    return std::make_unique<FileSystemRepository>(spec.location);
}

void validateMigration(const RepositorySpec& source, const RepositorySpec& target)
{
    std::error_code ec;
    if (fs::equivalent(source.location, target.location, ec))
    {
        throw std::invalid_argument(
            std::format("Can't migrate '{}' into itself", toString(source)));
    }

    // Writing inside a folder source would add bogus users to it mid-read
    if (source.type == RepositoryType::FileSystem && isInside(source, target))
    {
        throw std::invalid_argument(
            std::format("Target '{}' is inside the source '{}'",
                        toString(target),
                        toString(source)));
    }

    if (!fs::exists(target.location))
    {
        return;
    }

    if (target.type == RepositoryType::FileSystem)
    {
        if (!fs::is_directory(target.location))
        {
            throw std::invalid_argument(
                std::format("Target '{}' is not a folder", toString(target)));
        }

        if (!fs::is_empty(target.location))
        {
            throw std::invalid_argument(
                std::format("Target '{}' is not empty, migrating into it would "
                            "duplicate entries",
                            toString(target)));
        }
    }
    else if (!fs::is_regular_file(target.location))
    {
        throw std::invalid_argument(
            std::format("Target '{}' is not a file", toString(target)));
    }
}

} // namespace km
