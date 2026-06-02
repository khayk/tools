#pragma once

#include <duplicates/IDuplicates.h>
#include <duplicates/Node.h>

#include <unordered_map>
#include <unordered_set>
#include <map>

namespace tools::dups {

class DuplicateDetector
    : public IDuplicateDetector
    , public IDuplicateFiles
    , public IDuplicateGroups
{
public:
    DuplicateDetector();
    ~DuplicateDetector() = default;

    void addFile(const fs::path& path) override;

    size_t numFiles() const noexcept override;
    size_t numGroups() const noexcept override;

    void detect(const Options& opts, const ProgressCallback& cb) override;

    void enumFiles(const FileCallback& cb) const override;
    void enumGroups(const DupGroupCallback& cb) const override;

    const Node* root() const;
    void reset();

private:
    using Nodes = std::vector<const Node*>;
    using MapBySize = std::map<size_t, Nodes, std::greater<>>;
    using MapByHash = std::unordered_map<std::string_view, Nodes>;
    using PathTable = std::unordered_set<fs::path>;

    // Groups every leaf within the configured size bounds by file size. Files
    // whose size differs cannot be duplicates, so this is a cheap first pass
    // that avoids hashing the bulk of the input.
    MapBySize groupBySize(const Options& opts) const;

    // Drops size groups holding a single file (a unique size has no duplicate)
    // and accumulates the total byte volume left to hash into outstandingSize,
    // used to drive Stage::Calculate progress.
    static void pruneUniqueSizes(MapBySize& bySize, size_t& outstandingSize);

    // Hashes each surviving size group and records the content-identical files
    // directly into grps_, the single source of truth for duplicate groups.
    void buildHashGroups(const MapBySize& bySize,
                         size_t outstandingSize,
                         const ProgressCallback& cb);

    // Collapses hard links (paths sharing a device+inode) to a single
    // representative per group, dropping groups left with fewer than two files.
    void collapseHardLinks();

    PathTable names_;
    NodePtr root_;
    MapByHash grps_;
};

constexpr std::string_view stage2str(Stage stage)
{
    switch (stage)
    {
        case Stage::Prepare:
            return "Prepare";
        case Stage::Calculate:
            return "Calculate";
    }
    return "Unknown";
}

} // namespace tools::dups
