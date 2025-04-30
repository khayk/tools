#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <spdlog/spdlog.h>
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

DuplicateDetector::DuplicateDetector()
{
    reset();
}

DuplicateDetector::~DuplicateDetector() {}

void DuplicateDetector::addFile(const fs::path& path)
{
    Node* node = root_.get();
    auto separator = fs::path::preferred_separator;
    std::wstring wpath = path.lexically_normal().wstring();
    std::wstring_view wsv = wpath;
    std::wstring name;

    while (!wsv.empty())
    {
        auto p = wsv.find_first_of(separator);
        name = wsv.substr(0, p);
        auto [it, ok] = names_.insert(name);

        node = node->addChild(*it);

        if (p == std::wstring_view::npos)
        {
            break;
        }

        wsv.remove_prefix(p + 1);
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

void DuplicateDetector::detect(const Options& opts, ProgressCallback cb)
{
    dups_.clear();
    grps_.clear();

    size_t totalFiles = numFiles();

    if (totalFiles == 0)
    {
        return;
    }

    root_->update([i = 0U, totalFiles, &cb](const Node* node) mutable {
        cb(Stage::Prepare, node, ++i * 100 / totalFiles);
    });

    std::wstring ws;
    root_->enumLeafs([&opts, &ws, this](Node* node) {
        if (node->size() < opts.minSizeBytes || node->size() > opts.maxSizeBytes)
        {
            return;
        }

        node->fullPath(ws);
        auto it = dups_.find(node->size());

        if (it == dups_.end())
        {
            dups_.emplace(node->size(), Nodes {node});
        }
        else
        {
            it->second.push_back(node);
        }
    });

    // Files with unique size can be quickly excluded
    size_t numUniqueFiles = 0;
    size_t outstandingSize = 0;

    util::eraseIf(dups_, [&numUniqueFiles, &outstandingSize](const auto& vt) {
        if (vt.second.size() < 2)
        {
            ++numUniqueFiles;
            return true;
        }
        outstandingSize += vt.second.size() * vt.second[0]->size();
        return false;
    });

    totalFiles -= numUniqueFiles;
    size_t processedSize = 0;

    // Here we have files with the same size
    util::eraseIf(
        dups_,
        [i = 0U, &cb, totalFiles, &processedSize, outstandingSize](auto& vt) mutable {
            std::map<std::string, Nodes> hashes;
            std::string sha256;

            Nodes& nodes = vt.second;
            for (const auto* node : nodes)
            {
                if (!tryGetSha256(node, sha256))
                {
                    continue;
                }

                auto it = hashes.find(sha256);
                processedSize += node->size();
                const auto percent = processedSize * 100 / outstandingSize;
                cb(Stage::Calculate, node, percent);

                if (it == hashes.end())
                {
                    hashes.emplace(sha256, Nodes {node});
                }
                else
                {
                    it->second.push_back(node);
                }
            }

            // Remove all unique items
            util::eraseIf(hashes, [](const auto& vt) {
                return vt.second.size() < 2;
            });

            if (hashes.empty())
            {
                // Instruct to remove the current Node object
                return true;
            }

            // Remove files with unique hashes
            auto it =
                std::remove_if(std::begin(nodes),
                               std::end(nodes),
                               [&hashes](const Node* const node) {
                                   return hashes.find(node->sha256()) == hashes.end();
                               });

            nodes.erase(it, nodes.end());

            return false;
        });

    for (const auto& [sz, nodes] : dups_)
    {
        for (const auto* node : nodes)
        {
            grps_[node->sha256()].push_back(node);
        }
    }
}

void DuplicateDetector::reset()
{
    grps_.clear();
    dups_.clear();
    root_.reset();
    names_.clear();
    names_.emplace(L"");

    root_ = std::make_unique<Node>(*names_.begin());
}

void DuplicateDetector::enumFiles(const FileCallback& cb) const
{
    std::wstring ws;

    root_->enumLeafs([&ws, cb](const Node* const node) {
        node->fullPath(ws);
        cb(ws);
    });
}

void DuplicateDetector::enumDuplicates(const DupGroupCallback& cb) const
{
    DupGroup group;
    std::wstring ws;
    size_t duplicates = 0;
    std::unordered_set<std::string_view> visit;

    for (const auto& [sz, nodes] : dups_)
    {
        visit.clear();
        for (const auto* i : nodes)
        {
            visit.insert(i->sha256());
        }

        for (const auto& sha : visit)
        {
            group.groupId = ++duplicates;
            group.entires.clear();

            const auto it = grps_.find(sha);
            if (it == grps_.end())
            {
                continue;
            }

            const Nodes& nodesInGroup = it->second;
            for (const auto* i : nodesInGroup)
            {
                group.entires.emplace_back();
                DupEntry& e = group.entires.back();

                i->fullPath(ws);
                auto sv = ws;
                auto separator = fs::path::preferred_separator;
                auto p = sv.find_last_of(separator);
                auto filename = sv.substr(p + 1);
                auto folder = sv.substr(0, p);

                e.dir = folder;
                e.filename = filename;
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
}

const Node* DuplicateDetector::root() const
{
    return root_.get();
}

} // namespace tools::dups
