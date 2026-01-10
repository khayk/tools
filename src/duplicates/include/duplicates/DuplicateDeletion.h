#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/PathList.h>
#include <ostream>


namespace tools::dups {

using IgnoredPaths = PathList;

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

class DeletionConfig {
public:
    DeletionConfig(std::ostream& out, std::ostream& in);

    void setDeletionStrategy(IDeletionStrategy* strategy);
    void setDuplicateGroup(IDuplicateGroups* duplicates);
    void setProgress(Progress* progress);
    void setIgnoredPaths(const PathList& ignored);
    void setKeepFromPaths(const PathList& keepFrom);
    void setDeleteFromPaths(const PathList& deleteFrom);
};

/**
 * @brief Deletes duplicate files based on the provided detector and deletion strategy.
 *
 * @param strategy The deletion strategy to use.
 * @param detector The duplicate detector containing the groups of duplicates.
 * @param dirsToDeleteFrom Directories where files can be safely deleted.
 * @param ignoredPaths Object to track ignored files.
 * @param progress Object for updating progress
 * @param out Output stream for messages.
 * @param in Input stream for user interaction.
 */
void deleteDuplicates(const IDeletionStrategy& strategy,
                      const IDuplicateGroups& duplicates,
                      const PathsVec& dirsToDeleteFrom,
                      IgnoredPaths& ignoredPaths,
                      Progress& progress,
                      std::ostream& out,
                      std::istream& in);

} // namespace tools::dups