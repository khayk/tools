#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/PathList.h>
#include <duplicates/Menu.h>
#include <cstdint>
#include <ostream>


namespace tools::dups {

enum class Purpose : std::int8_t
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
 * @brief Configuration for duplication deletion process
 */
class DeletionConfig
{
    const IDeletionStrategy& strategy_;
    std::ostream& out_;
    std::istream& in_;
    Progress& progress_;
    StreamRenderer& renderer_;

    IgnoredPaths ignored_;
    KeepFromPaths keepFrom_;
    DeleteFromPaths deleteFrom_;

public:
    /**
     * @brief Construct a new Deletion Config object
     *
     * @param strategy The deletion strategy to use
     * @param out Output stream interactive output
     * @param in Input stream to interact with user
     * @param progress Progress reporter
     * @param renderer Menu renderer instance
     */
    DeletionConfig(const IDeletionStrategy& strategy,
                   std::ostream& out,
                   std::istream& in,
                   Progress& progress,
                   StreamRenderer& renderer);

    const IDeletionStrategy& strategy() const;
    StreamRenderer& renderer();
    std::ostream& out();
    std::istream& in();
    Progress& progress();
    IgnoredPaths& ignoredPaths();
    KeepFromPaths& keepFromPaths();
    DeleteFromPaths& deleteFromPaths();
};

enum class Flow : std::uint8_t
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
 * @param cfg Settings for deletion process
 *
 * @return Done if the operation was successful,
           Retry if the operation need to be retried again
           Quit if the user selected quit
 */
Flow deleteInteractively(PathsVec& files, DeletionConfig& cfg);


/**
 * @brief Deletes duplicate files based on the provided config
 *
 * @param duplicates The groups of duplicates
 * @param cfg Settings for deletion process
 */
void deleteDuplicates(const IDuplicateGroups& duplicates, DeletionConfig& cfg);

} // namespace tools::dups