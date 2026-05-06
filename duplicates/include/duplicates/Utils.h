#pragma once

#include <map>
#include <cstdint>
#include <cstddef>
#include <iosfwd>

namespace tools::dups {

class Node;

namespace util {

uint16_t digits(size_t n);

/**
 * @brief Print the content as a tree
 *
 * @param os The output stream
 */
void outputTree(const Node* root, std::ostream& os);

} // namespace util
} // namespace tools::dups
