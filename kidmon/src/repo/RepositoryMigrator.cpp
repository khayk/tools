#include <kidmon/repo/RepositoryMigrator.h>
#include <kidmon/repo/Filter.h>

#include <spdlog/spdlog.h>

#include <vector>

namespace km {

MigrationStats migrate(const IRepository& source,
                       IRepository& destination,
                       const MigrationProgressCb& onProgress,
                       const std::size_t chunkSize)
{
    MigrationStats stats;
    bool cancelled = false;

    std::vector<Entry> chunk;
    chunk.reserve(chunkSize);

    // Isolates the bad entries of a failed chunk so the rest still land.
    const auto copyOneByOne = [&](const std::vector<Entry>& entries) {
        for (const auto& entry : entries)
        {
            try
            {
                destination.add(entry);
                ++stats.entriesCopied;
            }
            catch (const std::exception& ex)
            {
                ++stats.entriesFailed;
                spdlog::error("migrate: failed to copy an entry for user '{}': {}",
                              entry.username,
                              ex.what());
            }
        }
    };

    // Returns false once the progress callback asks to cancel.
    const auto flush = [&] {
        if (chunk.empty())
        {
            return true;
        }

        try
        {
            destination.addAll(chunk);
            stats.entriesCopied += chunk.size();
        }
        catch (const std::exception& ex)
        {
            spdlog::warn("migrate: chunk of {} entries failed ({}); retrying "
                         "one by one",
                         chunk.size(),
                         ex.what());
            copyOneByOne(chunk);
        }
        chunk.clear();

        return !onProgress || onProgress(stats);
    };

    source.queryUsers([&](const std::string& username) {
        if (cancelled)
        {
            return false;
        }

        ++stats.usersSeen;

        const Filter filter(username); // default bounds cover the full history
        source.queryEntries(filter, [&](Entry& entry) {
            chunk.push_back(entry); // the source may reuse `entry`
            if (chunk.size() >= chunkSize && !flush())
            {
                cancelled = true;
            }
            return !cancelled;
        });

        return !cancelled;
    });

    if (!cancelled)
    {
        flush();
    }

    return stats;
}

} // namespace km
