#pragma once

#include <kidmon/repo/RepositoryFactory.h>

namespace km::reports {

/**
 * Throws std::invalid_argument if migrating into the target is unsafe: it is the
 * source itself, or a non-empty file system repository. The latter only appends,
 * so a repeated migration would duplicate every entry. A SQLite target may hold
 * data already, as it skips entries it has.
 */
void validateMigration(const RepositorySpec& source, const RepositorySpec& target);

} // namespace km::reports
