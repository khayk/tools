#pragma once

#include <functional>
#include <filesystem>
#include <vector>
#include <string>
#include <limits>

namespace fs = std::filesystem;

namespace tools::dups {

// @todo:hayk - reconsider presence of this class as part of progress callback
class Node;

struct DupEntry
{
    fs::path file;
    size_t size {};
    std::string sha256;
};

struct DupGroup
{
    size_t groupId {};
    std::vector<DupEntry> entires;
};

struct Options
{
    size_t minSizeBytes {};
    size_t maxSizeBytes {std::numeric_limits<size_t>::max()};
};

enum class Stage
{
    Prepare,
    Calculate
};

using FileCallback = std::function<void(const fs::path&)>;
using DupGroupCallback = std::function<bool(const DupGroup&)>;
using ProgressCallback = std::function<void(const Stage, const Node*, size_t)>;

extern const ProgressCallback& defaultProgressCallback;

class IDuplicateDetector
{
public:
    virtual ~IDuplicateDetector() = default;

    virtual void detect(const Options& opts,
                        const ProgressCallback& cb = defaultProgressCallback) = 0;
};


class IDuplicateGroups
{
public:
    virtual ~IDuplicateGroups() = default;

    virtual size_t numGroups() const noexcept = 0;

    virtual void enumGroups(const DupGroupCallback& cb) const = 0;
};


class IDuplicateFiles
{
public:
    virtual ~IDuplicateFiles() = default;

    virtual void addFile(const fs::path& path) = 0;

    virtual size_t numFiles() const noexcept = 0;

    virtual void enumFiles(const FileCallback& cb) const = 0;
};


} // namespace tools::dups
