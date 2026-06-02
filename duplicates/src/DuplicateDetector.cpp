#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <core/utils/File.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <system_error>

namespace tools::dups {
namespace {

bool tryGetSha256(const Node* node, std::string& sha256)
{
    try
    {
        sha256 = node->sha256();
        return true;
    }
    catch (const std::system_error& se)
    {
        spdlog::error("std::system_error: {}", se.what());
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }

    return false;
}
} // namespace

const ProgressCallback& defaultProgressCallback =
    [](const Stage, const Node*, size_t) {};

DuplicateDetector::DuplicateDetector()
{
    reset();
}

void DuplicateDetector::addFile(const fs::path& path)
{
    Node* node = root_.get();

    for (const auto& p : path)
    {
        auto [it, _] = names_.insert(p);
        node = node->addChild(*it);
    }
}

size_t DuplicateDetector::numFiles() const noexcept
{
    return !root_->leaf() ? root_->leafsCount() : 0;
}

size_t DuplicateDetector::numGroups() const noexcept
{
    return grps_.size();
}

void DuplicateDetector::detect(const Options& opts, const ProgressCallback& cb)
{
    grps_.clear();

    const size_t totalFiles = numFiles();

    if (totalFiles == 0)
    {
        return;
    }

    root_->update([i = 0UL, totalFiles, &cb](const Node* node) mutable {
        cb(Stage::Prepare, node, ++i * 100 / totalFiles);
    });

    // dups_ is a transient size index: files of differing size can never be
    // duplicates, so size grouping cheaply narrows down the hashing candidates.
    // It is intentionally local - grps_ is the single source of truth for groups.
    MapBySize bySize = groupBySize(opts);

    size_t outstandingSize = 0;
    pruneUniqueSizes(bySize, outstandingSize);

    buildHashGroups(bySize, outstandingSize, cb);

    collapseHardLinks();
}

DuplicateDetector::MapBySize DuplicateDetector::groupBySize(const Options& opts) const
{
    MapBySize bySize;

    root_->enumLeafs([&opts, &bySize](Node* node) {
        if (node->size() < opts.minSizeBytes || node->size() > opts.maxSizeBytes)
        {
            return;
        }

        bySize[node->size()].push_back(node);
    });

    return bySize;
}

void DuplicateDetector::pruneUniqueSizes(MapBySize& bySize, size_t& outstandingSize)
{
    // Files with a unique size can be quickly excluded
    std::erase_if(bySize, [&outstandingSize](const auto& vt) {
        const Nodes& nodes = vt.second;
        if (nodes.size() < 2)
        {
            return true;
        }
        outstandingSize += nodes.size() * nodes[0]->size();
        return false;
    });
}

void DuplicateDetector::buildHashGroups(const MapBySize& bySize,
                                        size_t outstandingSize,
                                        const ProgressCallback& cb)
{
    // Process the lightest size groups first (weight = size * count) so the
    // Stage::Calculate progress advances smoothly instead of in uneven jumps.
    std::vector<const Nodes*> ordered;
    ordered.reserve(bySize.size());

    for (const auto& [sz, nodes] : bySize)
    {
        ordered.push_back(&nodes);
    }

    std::ranges::sort(ordered, [](const Nodes* a, const Nodes* b) {
        return (a->front()->size() * a->size()) < (b->front()->size() * b->size());
    });

    // Keyed by content hash; the string_view borrows each Node's cached sha256_,
    // whose lifetime outlives detection.
    std::unordered_map<std::string_view, Nodes> hashes;
    std::string sha256;
    size_t processedSize = 0;

    for (const Nodes* group : ordered)
    {
        hashes.clear();

        // Here we have files with the same size
        for (const auto* node : *group)
        {
            if (!tryGetSha256(node, sha256))
            {
                continue;
            }

            processedSize += node->size();
            cb(Stage::Calculate, node, processedSize * 100 / outstandingSize);

            hashes[node->sha256()].push_back(node);
        }

        // A hash shared by two or more files is a real duplicate group; record
        // it straight into grps_ and discard the unique remainder.
        for (auto& [sha, nodes] : hashes)
        {
            if (nodes.size() >= 2)
            {
                grps_.emplace(sha, std::move(nodes));
            }
        }
    }
}

void DuplicateDetector::collapseHardLinks()
{
    // Hard links to the same physical file (same device + inode) share storage,
    // so "deleting" one reclaims nothing and only confuses the user. Within each
    // group keep a single representative per physical file, then drop any group
    // left with fewer than two distinct files - it is no longer a duplicate.
    std::erase_if(grps_, [](auto& kv) {
        Nodes& nodes = kv.second;
        std::unordered_set<core::file::FileId> seen;

        auto last = std::remove_if(nodes.begin(), nodes.end(), [&](const Node* node) {
            const auto id = core::file::fileId(node->fullPath());
            if (!id)
            {
                return false; // identity unknown: keep, treat as distinct
            }
            return !seen.insert(*id).second;
        });
        nodes.erase(last, nodes.end());

        return nodes.size() < 2;
    });
}

void DuplicateDetector::reset()
{
    grps_.clear();
    names_.clear();
    names_.emplace();
    root_ = std::make_unique<Node>(&(*names_.begin()));
}

void DuplicateDetector::enumFiles(const FileCallback& cb) const
{
    fs::path p;

    root_->enumLeafs([&p, cb](const Node* const node) {
        node->fullPath(p);
        cb(p);
    });
}

void DuplicateDetector::enumGroups(const DupGroupCallback& cb) const
{
    // grps_ is hash-keyed and therefore unordered; present the groups with the
    // largest files first, the order callers rely on.
    std::vector<const Nodes*> ordered;
    ordered.reserve(grps_.size());

    for (const auto& [sha, nodes] : grps_)
    {
        ordered.push_back(&nodes);
    }

    std::ranges::sort(ordered, [](const Nodes* a, const Nodes* b) {
        return a->front()->size() > b->front()->size();
    });

    DupGroup group;
    size_t duplicates = 0;

    for (const Nodes* nodes : ordered)
    {
        group.groupId = ++duplicates;
        group.entries.clear();

        for (const auto* i : *nodes)
        {
            group.entries.emplace_back();
            DupEntry& e = group.entries.back();

            i->fullPath(e.file);
            e.size = i->size();
            e.sha256 = i->sha256();
        }

        // Stop enumeration if the callback returns false
        if (!cb(group))
        {
            return;
        }
    }
}

const Node* DuplicateDetector::root() const
{
    return root_.get();
}

} // namespace tools::dups
