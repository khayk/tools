#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <core/utils/File.h>
#include <core/utils/Parallel.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace tools::dups {
namespace {

// Same underlying type as DuplicateDetector::Nodes; usable from free helpers.
using NodeVec = std::vector<const Node*>;

// Bytes hashed by the screening pass. Distinct files almost always differ
// within their first few kilobytes, so a prefix hash rules them out before any
// full read.
constexpr std::size_t PREFIX_BYTES = std::size_t {16} * 1024;

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

bool tryGetPrefixSha256(const Node* node, std::string& sha256)
{
    try
    {
        sha256 = node->prefixSha256(PREFIX_BYTES);
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

// Splits each input bucket into sub-buckets that agree on hashFn, dropping any
// sub-bucket left with fewer than two files. The expensive hashing runs in
// parallel across every candidate; the regrouping afterwards is cheap and
// serial. Each input bucket is assumed to already share a single file size, so
// a hash match can only mean identical content.
//
// hashFn(node, out) returns true and fills `out` on success, or false to drop
// the node. progressFn(node) runs once per hashed node and must be thread-safe.
template <typename HashFn, typename ProgressFn>
std::vector<NodeVec> refineByHash(const std::vector<NodeVec>& buckets,
                                  HashFn hashFn,
                                  ProgressFn progressFn)
{
    // Flatten candidates, remembering which bucket each came from. Buckets are
    // emitted contiguously, so the regrouping below can walk them in one pass.
    NodeVec cand;
    std::vector<std::size_t> bucketOf;
    for (std::size_t b = 0; b < buckets.size(); ++b)
    {
        for (const Node* node : buckets[b])
        {
            cand.push_back(node);
            bucketOf.push_back(b);
        }
    }

    std::vector<std::string> digests(cand.size());
    std::vector<char> ok(cand.size(), 0);

    core::parallelFor(cand.size(), [&](std::size_t i) {
        if (hashFn(cand[i], digests[i]))
        {
            ok[i] = 1;
        }
        progressFn(cand[i]);
    });

    std::vector<NodeVec> refined;
    std::unordered_map<std::string_view, NodeVec> sub;

    std::size_t i = 0;
    while (i < cand.size())
    {
        const std::size_t b = bucketOf[i];
        sub.clear();

        for (; i < cand.size() && bucketOf[i] == b; ++i)
        {
            if (ok[i])
            {
                // The key borrows digests[i], stable until this function returns.
                sub[digests[i]].push_back(cand[i]);
            }
        }

        for (auto& [digest, nodes] : sub)
        {
            if (nodes.size() >= 2)
            {
                refined.push_back(std::move(nodes));
            }
        }
    }

    return refined;
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

    // bySize is a transient size index: files of differing size can never be
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
    // Seed the buckets with the size groups: each already holds >= 2 files that
    // share one common size.
    std::vector<Nodes> buckets;
    buckets.reserve(bySize.size());
    for (const auto& [sz, nodes] : bySize)
    {
        buckets.push_back(nodes);
    }

    // Progress is reported from worker threads, so serialise the callback and
    // accumulate hashed bytes atomically. outstandingSize is the combined full
    // size of all candidates; the screening pass reads only a prefix of each, so
    // the running percentage is clamped to 100.
    std::atomic<size_t> processed {0};
    std::mutex cbMutex;
    auto report = [&](const Node* node, size_t bytes) {
        const size_t done =
            processed.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        const size_t percent =
            outstandingSize != 0 ? std::min<size_t>(done * 100 / outstandingSize, 100)
                                 : 100;
        const std::scoped_lock lock(cbMutex);
        cb(Stage::Calculate, node, percent);
    };

    // Stage A: cheap prefix screening. Files that differ within their first
    // PREFIX_BYTES are ruled out here without ever being read in full - the
    // decisive win for large media that merely share a size.
    buckets = refineByHash(buckets, tryGetPrefixSha256, [&](const Node* node) {
        report(node, std::min<size_t>(node->size(), PREFIX_BYTES));
    });

    // Stage B: authoritative full-file SHA-256 on the survivors. tryGetSha256
    // also warms each node's cached hash, which enumGroups() reuses.
    buckets = refineByHash(buckets, tryGetSha256, [&](const Node* node) {
        report(node, node->size());
    });

    // Every surviving bucket is a confirmed duplicate group, keyed by its now
    // cached full hash (a string_view into the front node's stable sha256_).
    for (auto& bucket : buckets)
    {
        grps_.emplace(bucket.front()->sha256(), std::move(bucket));
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
