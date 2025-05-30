#pragma once

#include <duplicates/Node.h>

#include <limits>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <map>

namespace fs = std::filesystem;

namespace tools::dups {

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

class DuplicateDetector
{
public:
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

    using FileCallback     = std::function<void(const fs::path&)>;
    using DupGroupCallback = std::function<bool(const DupGroup&)>;
    using ProgressCallback = std::function<void(const Stage stage,
                                                const Node* node,
                                                size_t percent)>;

    static ProgressCallback defaultProgressCallback()
    {
        return [](const Stage, const Node*, size_t) {};
    }

    DuplicateDetector();
    ~DuplicateDetector() = default;

    void addFile(const fs::path& path);

    size_t numFiles() const noexcept;
    size_t numGroups() const noexcept;

    void detect(const Options& opts, ProgressCallback cb = defaultProgressCallback());
    void reset();

    void enumFiles(const FileCallback& cb) const;
    void enumDuplicates(const DupGroupCallback& cb) const;

    const Node* root() const;

private:
    using Nodes = std::vector<const Node*>;
    using MapBySize = std::map<size_t, Nodes, std::greater<>>;
    using MapByHash = std::map<std::string_view, Nodes>;
    using PathTable = std::unordered_set<fs::path>;

    PathTable names_;
    NodePtr root_;
    MapBySize dups_;
    MapByHash grps_;
};

constexpr std::string_view stage2str(DuplicateDetector::Stage stage)
{
    switch (stage)
    {
        case DuplicateDetector::Stage::Prepare:
            return "Prepare";
        case DuplicateDetector::Stage::Calculate:
            return "Calculate";
    }
    return "Unknown";
}

} // namespace tools::dups
