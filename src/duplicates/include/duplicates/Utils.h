#pragma once

#include <duplicates/Config.h>
#include <duplicates/Progress.h>

#include <map>
#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <filesystem>

namespace tools::dups {

class Node;
class DuplicateDetector;

namespace fs = std::filesystem;

namespace util {

uint16_t digits(size_t n);

template <class Key, class T, class Compare, class Alloc, class Pred>
void eraseIf(std::map<Key, T, Compare, Alloc>& c, Pred pred)
{
    for (auto i = c.begin(), last = c.end(); i != last;)
    {
        if (pred(*i))
        {
            i = c.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

/**
 * @brief Print the content as a tree
 *
 * @param os The output stream
 */
void treeDump(const Node* root, std::ostream& os);


/**
 * @brief Dump the content of all files scanned by the DuplicateDetector
 *
 * @param allFiles The path to the file where to dump the content
 * @param detector The DuplicateDetector instance containing the scanned files
 */
void dumpContent(const fs::path& allFiles, const DuplicateDetector& detector);

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

} // namespace util
} // namespace tools::dups
