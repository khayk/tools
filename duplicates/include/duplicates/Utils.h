#pragma once

#include <cstddef>
#include <iosfwd>

namespace tools::dups {

class Node;

namespace util {

/**
 * @brief Print the content as a tree
 *
 * @param os The output stream
 */
void outputTree(const Node* root, std::ostream& os);

} // namespace util
} // namespace tools::dups
