#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Progress.h>
#include <duplicates/Config.h>

#include <vector>
#include <unordered_set>
#include <filesystem>

namespace tools::dups {

using PathsVec = std::vector<fs::path>;
using PathsSet = std::unordered_set<fs::path>;
namespace fs = std::filesystem;

class IgnoredFiles
{
public:
    IgnoredFiles() = default;
    IgnoredFiles(fs::path file, bool saveWhenDone = true);
    ~IgnoredFiles();

    const PathsSet& files() const;

    bool contains(const fs::path& file) const;

    bool empty() const noexcept;

    size_t size() const noexcept;

    void add(const fs::path& file);

    void add(const PathsVec& files);

private:
    /**
     * @brief Saves the ignored files to the specified file.
     *
     * @param files The set of file paths to save.
     */
    void save() const;

    /**
     * @brief Loads ignored files from the specified file.
     *
     * @param out Output stream for messages.
     * @return A set of ignored file paths.
     */
    void load();

    fs::path filePath_;
    PathsSet files_;
    bool saveWhenDone_{false};
};

/**
 * @brief Deletes files using the provided deletion strategy.
 *
 * @param strategy The deletion strategy to use.
 * @param files The vector of file paths to delete.
 */
void deleteFiles(const IDeletionStrategy& strategy, PathsVec& files);


/**
 * @brief Deletes files interactively, allowing the user to choose which files to delete.
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
    * @param safeToDeleteDirs Directories where files can be safely deleted.
    * @param ignoredFiles Object to track ignored files.
    * @param progress Object for updating progress
    * @param out Output stream for messages.
    * @param in Input stream for user interaction.
 */
void deleteDuplicates(const IDeletionStrategy& strategy,
                      const IDuplicateGroups& duplicates,
                      const PathsVec& safeToDeleteDirs,
                      IgnoredFiles& ignoredFiles,
                      Progress& progress,
                      std::ostream& out,
                      std::istream& in);

} // namespace tools::dups