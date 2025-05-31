#include <duplicates/Node.h>
#include <core/utils/Crypto.h>

#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

namespace tools::dups {
namespace detail {

void fullPathHelper(const Node* node, fs::path& dest)
{
    if (node != nullptr)
    {
        fullPathHelper(node->parent(), dest);

        if (node->parent() != nullptr)
        {
            dest /= node->name();
        }

        // if (!dest.empty() || (!node->name().empty() &&
        //                       (node->name().native().size() != 2 ||
        //                       node->name().native()[1] != ':')))
        // {
        //     dest.push_back(fs::path::preferred_separator);
        // }
    }
}

bool tryGetFileSize(const fs::path& p, size_t& size)
{
    std::error_code ec {};
    size = fs::file_size(p, ec);
    return !ec;
}

} // namespace detail

Node::Node(const fs::path* name, Node* parent)
    : name_(name)
    , parent_(parent)
    , depth_(parent ? parent->depth() + 1 : 0)
{
    assert(name_ != nullptr);
}

const fs::path& Node::name() const noexcept
{
    return *name_;
}

Node* Node::parent() const noexcept
{
    return parent_;
}

bool Node::leaf() const noexcept
{
    return children_.empty();
}

size_t Node::size() const noexcept
{
    return size_;
}

uint16_t Node::depth() const noexcept
{
    return depth_;
}

const std::string& Node::sha256() const
{
    if (sha256_.empty())
    {
        sha256_ = crypto::fileSha256(fullPath());
    }

    return sha256_;
}

void Node::fullPath(fs::path& path) const
{
    path.clear();
    detail::fullPathHelper(this, path);
}

fs::path Node::fullPath() const
{
    fs::path p;
    fullPath(p);

    return p;
}

bool Node::hasChild(const fs::path& name) const
{
    auto it = children_.find(&name);

    return it != children_.end();
}

Node* Node::addChild(const fs::path& name)
{
    auto [it, ok] = children_.emplace(&name, nullptr);

    if (ok)
    {
        it->second = std::make_unique<Node>(it->first, this);
    }

    return it->second.get();
}

void Node::enumLeafs(const ConstNodeCallback& cb) const
{
    if (children_.empty())
    {
        cb(this);
    }

    for (const auto& it : children_)
    {
        const Node* node = it.second.get();
        node->enumLeafs(cb);
    }
}

void Node::enumLeafs(const MutableNodeCallback& cb)
{
    if (children_.empty())
    {
        cb(this);
    }

    for (auto& it : children_)
    {
        Node* child = it.second.get();
        child->enumLeafs(cb);
    }
}

void Node::enumNodes(const ConstNodeCallback& cb) const
{
    if (depth() != 0)
    {
        cb(this);
    }

    // Non-leafs
    for (const auto& it : children_)
    {
        const Node* node = it.second.get();
        if (!node->leaf())
        {
            node->enumNodes(cb);
        }
    }

    // Leafs
    for (const auto& it : children_)
    {
        const Node* node = it.second.get();
        if (node->leaf())
        {
            node->enumNodes(cb);
        }
    }
}

size_t Node::nodesCount() const noexcept
{
    size_t count = 0;

    for (const auto& it : children_)
    {
        const auto& child = it.second;
        count += child->nodesCount();
    }

    return count + 1;
}

size_t Node::leafsCount() const noexcept
{
    if (leaf())
    {
        return 1;
    }

    size_t count = 0;

    for (const auto& it : children_)
    {
        const auto& child = it.second;
        count += child->leafsCount();
    }

    return count;
}

void Node::update(const UpdateCallback& cb)
{
    fs::path p;
    updateHelper(cb, this, p);
}

void Node::updateHelper(const UpdateCallback& cb, Node* node, fs::path& p)
{
    if (node == nullptr)
    {
        return;
    }

    if (node->children_.empty())
    {
        p.clear();
        node->fullPath(p);
        detail::tryGetFileSize(p, node->size_);
        cb(node);
        return;
    }

    node->size_ = 0;
    node->sha256_.clear();

    for (auto& it : node->children_)
    {
        Node* child = it.second.get();

        updateHelper(cb, child, p);
        node->size_ += child->size();
    }
}

} // namespace tools::dups
