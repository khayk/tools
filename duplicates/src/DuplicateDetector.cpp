#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "DuplicateDetector.h"
#include "StopWatch.h"
#include "Utils.h"

#include <iostream>
#include <iomanip>

DuplicateDetector::DuplicateDetector()
{
    names_.emplace(L"");
    root_ = std::make_unique<Node>(*names_.begin());
}

DuplicateDetector::~DuplicateDetector()
{
    std::cout << "Total nodes count: " << root_->nodesCount() << std::endl;
}

void DuplicateDetector::add(const fs::path& path)
{
    Node* node = root_.get();
    auto separator = fs::path::preferred_separator;
    std::wstring wpath = path.wstring();
    std::wstring_view wsv = wpath;

    while (!wsv.empty())
    {
        auto p = wsv.find_first_of(separator);

        std::wstring name(wsv.substr(0, p));
        auto [it, ok] = names_.emplace(std::move(name));

        node = node->addChild(*it);

        if (p == std::wstring_view::npos)
        {
            break;
        }

        wsv.remove_prefix(p + 1);
    }
}

size_t DuplicateDetector::files() const noexcept
{
    return root_->leafsCount();
}

size_t DuplicateDetector::groups() const noexcept
{
    return dups_.size();
}

void DuplicateDetector::detect(const Options& options)
{
    dups_.clear();
    
    std::wstring ws;
    root_->update();
    root_->enumLeafs([&ws, &options, this](Node* node)
        {
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
    eraseIf(dups_, [](const auto& vt) {
        return vt.second.size() < 2;
        });

    // Here we have files with the same size
    eraseIf(dups_, [](auto& vt) {
        std::map<std::string, Nodes> hashes;

        Nodes& nodes = vt.second;

        for (auto node : nodes)
        {
            auto it = hashes.find(node->sha256());

            if (it == hashes.end())
            {
                hashes.emplace(node->sha256(), Nodes {node});
            }
            else
            {
                it->second.push_back(node);
            }
        }

        // Remove all unique items
        eraseIf(hashes, [](const auto& vt) {
            return vt.second.size() < 2;
            });

        if (hashes.empty())
        {
            // Instruct to remove the current Nodes object
            return true;
        }

        // Remove files with unique hashes
        auto it = std::remove_if(std::begin(nodes), std::end(nodes),
            [&hashes](const Node* const node) {
                return hashes.find(node->sha256()) == hashes.end();
            });

        nodes.erase(it, nodes.end());

        return false;
        });
}

void DuplicateDetector::enumFiles(FileCallback cb) const
{
    std::wstring ws;

    root_->enumLeafs([&ws, cb](const Node* const node) {
        node->fullPath(ws);
        cb(ws);
        });
}

void DuplicateDetector::enumDuplicates(DupGroupCallback cb) const
{
    DupGroup group;
    std::wstring ws;
    size_t duplicates = 0;

    for (const auto& [k, v] : dups_)
    {
        group.groupId = ++duplicates;
        group.entires.clear();

        for (const auto& i : v)
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
            e.sha = i->sha256();
        }

        cb(group);
        ++duplicates;
    }
}
