#pragma once

#include "Repository.h"
#include <cstddef>
#include <functional>

namespace km {

struct MigrationStats
{
    std::size_t usersSeen {0};
    std::size_t entriesCopied {0};
    std::size_t entriesFailed {0};
};

// Called after each copy attempt with the running totals; return false to
// cancel the migration early.
using MigrationProgressCb = std::function<bool(const MigrationStats&)>;

/**
 * @brief Copies every entry from `source` into `destination`.
 *
 * Works purely through the IRepository interface, so it works between any two
 * repositories. A source read failure aborts the run; a destination write
 * failure for a single entry is logged and counted but does not.
 */
MigrationStats migrate(const IRepository& source,
                       IRepository& destination,
                       const MigrationProgressCb& onProgress = {});

} // namespace km
