#pragma once

#include <duplicates/IDuplicates.h>
#include <duplicates/Node.h>

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

    void detect(const Options& opts, ProgressCallback cb) override;

    void enumFiles(const FileCallback& cb) const override;
    void enumGroups(const DupGroupCallback& cb) const override;

    const Node* root() const;
    void reset();

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
