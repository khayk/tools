#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/PathList.h>
#include <cstdint>
#include <ostream>


namespace tools::dups {

enum class Purpose : std::int8_t {
    Ignored,
    KeepFrom,
    DeleteFrom
};

using IgnoredPaths = PathList<Purpose::Ignored>;
using KeepFromPaths = PathList<Purpose::KeepFrom>;
using DeleteFromPaths = PathList<Purpose::DeleteFrom>;

/**
 * @brief Deletes files using the provided deletion strategy.
 *
 * @param strategy The deletion strategy to use.
 * @param files The vector of file paths to delete.
 */
void deleteFiles(const IDeletionStrategy& strategy, PathsVec& files);


/**
 * @brief Deletes files interactively, allowing the user to choose which files to
 * delete.
 *
 * @param strategy The deletion strategy to use.
 * @param files The vector of file paths to delete.
 * @param ignoredPaths Object to track ignored files.
 * @param out Output stream for messages.
 * @param in Input stream for user interaction.
 *
 * @return true if the operation was successful, false otherwise.
 */
bool deleteInteractively(const IDeletionStrategy& strategy,
                         PathsVec& files,
                         IgnoredPaths& ignoredPaths,
                         std::ostream& out,
                         std::istream& in);

/**
 * @brief Configuration for duplication deletion process
 */
class DeletionConfig {
    const IDuplicateGroups* duplicates_ {};
    const IDeletionStrategy* strategy_ {};
    std::ostream* out_ {};
    std::istream* in_ {};
    Progress* progress_ {};
    IgnoredPaths* ignored_ {};
    KeepFromPaths* keepFrom_ {};
    DeleteFromPaths* deleteFrom_ {};

public:
    /**
    * @brief Construct a new Deletion Config object
    *
    * @param duplicates The groups of duplicates
    * @param strategy The deletion strategy to use
    * @param out Output stream interactive output
    * @param in Input stream to interact with user
    */
    DeletionConfig(const IDuplicateGroups& duplicates,
                   const IDeletionStrategy& strategy,
                   std::ostream& out, std::istream& in);

    /**
     * @brief Set the Progress object
     *
     * @param progress Object to deliver deletion progress
     */
    void setProgress(Progress& progress);

    /**
     * @brief Set the Ignored Paths object
     *
     * @param ignored Paths to ignored
     */
    void setIgnoredPaths(IgnoredPaths& ignored);

    /**
    * @brief Set the Keep From Paths object
    *
    * @param keepFrom Directories to use as desired locations to keep from
    */
    void setKeepFromPaths(KeepFromPaths& keepFrom);

    /**
     * @brief Set the Delete From Paths object
     *
     * @param deleteFrom Directories where files can be safely deleted
     */
    void setDeleteFromPaths(DeleteFromPaths& deleteFrom);

    const IDeletionStrategy& strategy() const;
    const IDuplicateGroups& duplicates() const;
    std::ostream& out();
    std::istream& in();

    Progress* progress();
    IgnoredPaths* ignoredPaths();
    KeepFromPaths* keepFromPaths();
    DeleteFromPaths* deleteFromPaths();
};

/**
 * @brief Deletes duplicate files based on the provided config
 *
 * @param cfg Settings for deletion process
 */
void deleteDuplicates(DeletionConfig& cfg);

} // namespace tools::dups