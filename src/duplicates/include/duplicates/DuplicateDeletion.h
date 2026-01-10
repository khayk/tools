#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>
#include <duplicates/FileList.h>


namespace tools::dups {

using IgnoredFiles = FileList;

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
 * @param ignoredFiles Object to track ignored files.
 * @param out Output stream for messages.
 * @param in Input stream for user interaction.
 *
 * @return true if the operation was successful, false otherwise.
 */
bool deleteInteractively(const IDeletionStrategy& strategy,
                         PathsVec& files,
                         IgnoredFiles& ignoredFiles,
                         std::ostream& out,
                         std::istream& in);

/**
 * @brief Deletes duplicate files based on the provided detector and deletion strategy.
 *
 * @param strategy The deletion strategy to use.
 * @param detector The duplicate detector containing the groups of duplicates.
 * @param dirsToDeleteFrom Directories where files can be safely deleted.
 * @param ignoredFiles Object to track ignored files.
 * @param progress Object for updating progress
 * @param out Output stream for messages.
 * @param in Input stream for user interaction.
 */
void deleteDuplicates(const IDeletionStrategy& strategy,
                      const IDuplicateGroups& duplicates,
                      const PathsVec& dirsToDeleteFrom,
                      IgnoredFiles& ignoredFiles,
                      Progress& progress,
                      std::ostream& out,
                      std::istream& in);

} // namespace tools::dups