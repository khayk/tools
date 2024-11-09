#pragma once

#include <kidmon/data/Types.h>

class ProcNameKeyBuilder
{
public:
    std::string operator()(const Entry& entry)
    {
        return file::path2s(entry.processInfo.processPath.filename());
    }
};

class ProcPathKeyBuilder
{
public:
    std::string operator()(const Entry& entry)
    {
        return file::path2s(entry.processInfo.processPath);
    }
};

class TitleKeyBuilder
{
public:
    std::string operator()(const Entry& entry)
    {
        return entry.windowInfo.title;
    }
};

class DescendingOrder
{
public:
    bool operator()(const Data& lhs, const Data& rhs) const noexcept
    {
        return lhs.duration() > rhs.duration();
    }
};

