#pragma once

#include <map>

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
