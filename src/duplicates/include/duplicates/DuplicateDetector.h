#pragma once

#include "Node.h"

#include <unordered_set>
#include <filesystem>
#include <vector>
#include <map>

namespace fs = std::filesystem;

namespace tools {
namespace dups {

struct DupEntry
{
    fs::path dir;
    fs::path filename;
    size_t size {};
    std::string sha256;
};

struct DupGroup
{
    size_t groupId {};
    std::vector<DupEntry> entires;
};

class DuplicateDetector
{
public:
    using FileCallback = std::function<void(const fs::path&)>;
    using DupGroupCallback = std::function<void(const DupGroup&)>;

    struct Options
    {
        size_t minSize {};
        size_t maxSize {};
    };

    DuplicateDetector();
    ~DuplicateDetector();

    void addFile(const fs::path& path);
    
    size_t numFiles() const noexcept;
    size_t numGroups() const noexcept;

    void detect(const Options& options);
    void reset();

    void enumFiles(const FileCallback& cb) const;
    void enumDuplicates(const DupGroupCallback& cb) const;

private:
    using Nodes = std::vector<const Node*>;
    using MapBySize = std::map<size_t, Nodes>;

    std::unordered_set<std::wstring> names_;
    std::unique_ptr<Node> root_;
    MapBySize dups_;
};

} // namespace dups
} // namespace tools
