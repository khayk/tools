#include <kidmon/repo/RepositoryMigrator.h>
#include <kidmon/repo/Filter.h>

#include <spdlog/spdlog.h>

namespace km {

MigrationStats migrate(const IRepository& source,
                       IRepository& destination,
                       const MigrationProgressCb& onProgress)
{
    MigrationStats stats;
    bool cancelled = false;

    source.queryUsers([&](const std::string& username) {
        if (cancelled)
        {
            return false;
        }

        ++stats.usersSeen;

        const Filter filter(username); // default bounds cover the full history
        source.queryEntries(filter, [&](Entry& entry) {
            try
            {
                destination.add(entry);
                ++stats.entriesCopied;
            }
            catch (const std::exception& ex)
            {
                ++stats.entriesFailed;
                spdlog::error("migrate: failed to copy an entry for user '{}': {}",
                              username,
                              ex.what());
            }

            if (onProgress && !onProgress(stats))
            {
                cancelled = true;
            }

            return !cancelled;
        });

        return !cancelled;
    });

    return stats;
}

} // namespace km
