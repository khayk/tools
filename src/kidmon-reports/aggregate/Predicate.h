#pragma once

#include <kidmon/data/Types.h>
#include <core/utils/File.h>

class ProcNameBuilder
{
public:
    std::string_view field() const noexcept
    {
        return "proc_name";
    }

    std::string value(const Entry& entry) const
    {
        return file::path2s(entry.processInfo.processPath.filename());
    }
};

class ProcPathBuilder
{
public:
    std::string_view field() const noexcept
    {
        return "proc_path";
    }

    std::string value(const Entry& entry) const
    {
        return file::path2s(entry.processInfo.processPath);
    }
};

class TitleBuilder
{
public:
    std::string_view field() const noexcept
    {
        return "title";
    }

    std::string value(const Entry& entry) const
    {
        return entry.windowInfo.title;
    }
};
