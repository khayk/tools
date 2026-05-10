#pragma once

#include <kidmon/data/Types.h>
#include <core/utils/File.h>

class ProcNameBuilder
{
public:
    static std::string_view field() noexcept
    {
        return "proc_name";
    }

    static std::string value(const km::Entry& entry)
    {
        return core::file::path2s(entry.processInfo.processPath.filename());
    }
};

class ProcPathBuilder
{
public:
    static std::string_view field() noexcept
    {
        return "proc_path";
    }

    static std::string value(const km::Entry& entry)
    {
        return core::file::path2s(entry.processInfo.processPath);
    }
};

class TitleBuilder
{
public:
    static std::string_view field() noexcept
    {
        return "title";
    }

    static std::string value(const km::Entry& entry)
    {
        return entry.windowInfo.title;
    }
};
