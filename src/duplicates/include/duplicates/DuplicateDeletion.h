#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/PathList.h>
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
     */
    DeletionConfig(const IDeletionStrategy& strategy,
                   std::ostream& out,
                   std::istream& in,
                   Progress& progress);

    const IDeletionStrategy& strategy() const;
    std::ostream& out();
    std::istream& in();
    Progress& progress();
    IgnoredPaths& ignoredPaths();
    KeepFromPaths& keepFromPaths();
    DeleteFromPaths& deleteFromPaths();
};


/**
 * @brief Deletes files interactively, allowing the user to choose which files to
 * delete.
 *
 * @param files The vector of file paths to delete.
 * @param cfg Settings for deletion process
 *
 * @return true if the operation was successful, false otherwise.
 */
bool deleteInteractively(PathsVec& files, DeletionConfig& cfg);


/**
 * @brief Deletes duplicate files based on the provided config
 *
 * @param duplicates The groups of duplicates
 * @param cfg Settings for deletion process
 */
void deleteDuplicates(const IDuplicateGroups& duplicates, DeletionConfig& cfg);

} // namespace tools::dups