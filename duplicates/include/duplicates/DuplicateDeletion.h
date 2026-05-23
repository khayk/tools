#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/PathList.h>
#include <duplicates/Menu.h>
#include <cstdint>


namespace tools::dups {

enum class Purpose : uint8_t
{
    Ignored,
    KeepFrom,
    DeleteFrom
};

using IgnoredPaths = Paths<Purpose::Ignored>;
using KeepFromPaths = Paths<Purpose::KeepFrom>;
using DeleteFromPaths = Paths<Purpose::DeleteFrom>;

/**
 * @brief Deletes files using the provided deletion strategy.
 *
 * @param strategy The deletion strategy to use.
 * @param files The vector of file paths to delete.
 */
void deleteFiles(const IDeletionStrategy& strategy, PathsVec& files);


/**
 * @brief Mutable path lists that evolve during the interactive deletion session.
 *        Kept separate so PathsPersister can attach to a single, clearly-bounded
 *        object rather than to individual PathsSet references scattered inside
 *        DeletionContext.
 */
struct DeletionState
{
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    DeleteFromPaths deleteFrom;
};

/**
 * @brief Context for duplication deletion process
 */
class DeletionContext
{
    const IDeletionStrategy& strategy_;
    Progress& progress_;
    StreamIO& io_;
    DeletionState& state_;

public:
    /**
     * @brief Construct a new Deletion Context object
     *
     * @param strategy The deletion strategy to use
     * @param progress Progress reporter
     * @param io Menu input/output instance
     * @param state Mutable path lists for the session
     */
    DeletionContext(const IDeletionStrategy& strategy,
                   Progress& progress,
                   StreamIO& io,
                   DeletionState& state);

    const IDeletionStrategy& strategy() const noexcept;
    const Progress& progress() const noexcept;
    Progress& progress() noexcept;
    StreamIO& io() noexcept;
    IgnoredPaths& ignoredPaths() noexcept;
    KeepFromPaths& keepFromPaths() noexcept;
    DeleteFromPaths& deleteFromPaths() noexcept;
};

enum class Flow : uint8_t
{
    Retry,
    Done,
    Quit
};

/**
 * @brief Deletes files interactively, allowing the user to choose which files to
 * delete.
 *
 * @param files The vector of file paths to delete.
 * @param ctx Context for deletion process
 *
 * @return Done if the operation was successful,
           Retry if the operation need to be retried again
           Quit if the user selected quit
 */
Flow deleteInteractively(PathsVec& files, DeletionContext& ctx);


/**
 * @brief Deletes duplicate files based on the provided config
 *
 * @param duplicates The groups of duplicates
 * @param ctx Context for deletion process
 */
void deleteDuplicates(const IDuplicateGroups& duplicates, DeletionContext& ctx);

} // namespace tools::dups