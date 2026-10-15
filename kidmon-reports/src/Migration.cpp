#include "Migration.h"

#include <format>
#include <stdexcept>

namespace km::reports {

void validateMigration(const RepositorySpec& source, const RepositorySpec& target)
{
    if (fs::weakly_canonical(source.location) == fs::weakly_canonical(target.location))
    {
        throw std::invalid_argument(
            std::format("Can't migrate '{}' into itself", toString(source)));
    }

    if (target.type == RepositoryType::FileSystem && fs::exists(target.location) &&
        !fs::is_empty(target.location))
    {
        throw std::invalid_argument(
            std::format("Target '{}' is not empty, migrating into it would duplicate "
                        "entries",
                        toString(target)));
    }
}

} // namespace km::reports
