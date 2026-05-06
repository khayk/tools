
#include <duplicates/Config.h>
#include <duplicates/Progress.h>
#include <duplicates/DuplicateDetector.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace tools::dups {

/**
 * @brief Scan directories for files and populate the DuplicateDetector
 *
 * @param cfg The configuration containing directories to scan and other settings
 * @param detector The DuplicateDetector instance to populate with scanned files
 * @param progress The Progress instance to get updates during the scan
 */
void scanDirectories(const Config& cfg,
                     DuplicateDetector& detector,
                     Progress& progress);

/**
 * @brief Dump the content of all files scanned by the DuplicateDetector
 *
 * @param allFiles The path to the file where to dump the content
 * @param detector The DuplicateDetector instance containing the scanned files
 */
void outputFiles(const fs::path& allFiles, const DuplicateDetector& detector);

/**
 * @brief Scan directories based on configuration
 *
 * @param cfg      The configuration containing directories to scan and other settings
 * @param detector The DuplicateDetector instance to populate with files
 * @param progress The Progress instance to get updates during the operation
 */
void detectDuplicates(const Config& cfg,
                      DuplicateDetector& detector,
                      Progress& progress);

/**
 * @brief Dump duplicates to a file
 *
 * @param reportPath The path to the file where to report duplicates
 * @param detector The DuplicateDetector instance containing detected duplicates
 */
void reportDuplicates(const fs::path& reportPath, const DuplicateDetector& detector);


} // namespace tools::dups
