#pragma once

#include "Node.h"

#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>

namespace fs = std::filesystem;

struct DupEntry
{
    std::wstring dir;
    std::wstring filename;
    size_t size;
    std::string sha;
};

struct DupGroup
{
    size_t groupId;
    std::vector<DupEntry> entires;
};

struct Options
{
    size_t minSize {0};
    size_t maxSize {0};
};

class DuplicateDetector
{
public:
    using FileCallback = std::function<void(const std::wstring&)>;
    using DupGroupCallback = std::function<void(const DupGroup&)>;

    DuplicateDetector();
    ~DuplicateDetector();

    void add(const fs::path& path);
    size_t files() const noexcept;
    size_t groups() const noexcept;

    void detect(const Options& options);

    void enumFiles(FileCallback cb) const;
    void enumDuplicates(DupGroupCallback cb) const;

    /**
     * @brief Print the content as a tree
     *
     * @param os The output stream
     */
    void treeDump(std::ostream& os);

private:
    using Nodes = std::vector<const Node*>;
    using MapBySize = std::map<size_t, Nodes>;

    std::unordered_set<std::wstring> names_;
    std::unique_ptr<Node> root_;
    MapBySize dups_;
};
