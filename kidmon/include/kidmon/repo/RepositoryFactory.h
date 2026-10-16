#pragma once

#include "Repository.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace km {

enum class RepositoryType : std::uint8_t
{
    FileSystem,
    Sqlite,
};

struct RepositorySpec
{
    RepositoryType type {RepositoryType::FileSystem};
    fs::path location;
};

/**
 * @brief Parses a repository spec in the form `[type:]location`, where type is
 * `fs` or `sqlite`.
 *
 * Without a type, it is inferred: an existing directory is a file system
 * repository, a `.db`, `.sqlite` or `.sqlite3` file is a SQLite one. Throws
 * std::invalid_argument when the type can't be determined.
 */
RepositorySpec parseRepositorySpec(std::string_view spec);

/**
 * @brief Formats the spec as `type:location`
 */
std::string toString(const RepositorySpec& spec);

enum class RepositoryAccess : std::uint8_t
{
    Read,  // the repository must already exist
    Write, // created if missing
};

/**
 * @brief Opens the repository described by the spec. Throws std::invalid_argument
 * if it is opened for reading and does not exist.
 */
std::unique_ptr<IRepository> openRepository(const RepositorySpec& spec,
                                            RepositoryAccess access);

/**
 * @brief Throws std::invalid_argument if migrating from source into target is
 * unsafe: the target is the source or lies inside it, has the wrong kind (a file
 * where a folder is expected or vice versa), or is a non-empty file system
 * repository. The latter only appends, so a repeated migration would duplicate
 * every entry; a SQLite target skips entries it already has.
 */
void validateMigration(const RepositorySpec& source, const RepositorySpec& target);

} // namespace km
