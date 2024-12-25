#pragma once

#include <map>
#include <cstdint>
#include <cstddef>
#include <iosfwd>

namespace tools {
namespace dups {

class Node;

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

} // namespace util
} // namespace dups
} // namespace tools
