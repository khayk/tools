#pragma once

#include <string>
#include <map>

std::wstring s2ws(const std::string& utf8);
std::string ws2s(const std::wstring& utf16);
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
