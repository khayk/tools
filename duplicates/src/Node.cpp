#include <duplicates/Node.h>
#include <core/utils/Crypto.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace detail {

void fullPathHelper(const Node* node, size_t size, std::wstring& dest)
{
    if (node == nullptr)
    {
        dest.reserve(size);
        return;
    }

    fullPathHelper(node->parent(), size + node->name().size() + 1, dest);

    if (!dest.empty())
    {
        dest.push_back(fs::path::preferred_separator);
    }

    dest.append(node->name());
}

size_t tryGetFileSize(const std::wstring& ws)
{
    std::error_code ec;

    return fs::file_size(fs::path(ws), ec);
}

} // namespace detail

Node::Node(std::wstring_view name, Node* parent)
    : name_(name)
    , parent_(parent)
    , size_(0)
    , depth_(parent ? parent->depth() + 1 : 0)
{
}

std::wstring_view Node::name() const noexcept
{
    return name_;
}

Node* Node::parent() const noexcept
{
    return parent_;
}

bool Node::leaf() const noexcept
{
    return childs_.size() == 0;
}

size_t Node::size() const noexcept
{
    return size_;
}

size_t Node::depth() const noexcept
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

void Node::fullPath(std::wstring& ws) const
{
    ws.clear();
    detail::fullPathHelper(this, 0, ws);
}

std::wstring Node::fullPath() const
{
    std::wstring ws;
    fullPath(ws);

    return ws;
}

bool Node::hasChild(std::wstring_view name) const
{
    auto it = childs_.find(name);

    return it != childs_.end();
}

Node* Node::addChild(std::wstring_view name)
{
    auto [it, ok] = childs_.emplace(name, nullptr);

    if (ok)
    {
        it->second = std::make_unique<Node>(it->first, this);
    }

    return it->second.get();
}

void Node::enumLeafs(ConstNodeCallback cb) const
{
    if (childs_.size() == 0)
    {
        cb(this);
    }

    for (const auto& it : childs_)
    {
        const Node* node = it.second.get();
        node->enumLeafs(cb);
    }
}

void Node::enumLeafs(MutableNodeCallback cb)
{
    if (childs_.size() == 0)
    {
        cb(this);
    }

    for (auto& it : childs_)
    {
        Node* child = it.second.get();
        child->enumLeafs(cb);
    }
}

void Node::enumNodes(ConstNodeCallback cb) const
{
    if (depth() != 0)
    {
        cb(this);
    }

    // Non-leafs
    for (const auto& it : childs_)
    {
        const Node* node = it.second.get();
        if (!node->leaf())
        {
            node->enumNodes(cb);
        }
    }

    // Leafs
    for (const auto& it : childs_)
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

    for (const auto& it : childs_)
    {
        const auto& child = it.second;
        count += child->nodesCount();
    }

    return count + 1;
}

size_t Node::leafsCount() const noexcept
{
    size_t count = 0;

    for (const auto& it : childs_)
    {
        const auto& child = it.second;
        count += child->leafsCount();
    }

    return count + ((childs_.size() == 0) ? 1 : 0);
}

void Node::update()
{
    std::wstring ws;
    updateHelper(this, ws);
}

void Node::updateHelper(Node* node, std::wstring& ws)
{
    if (node == nullptr)
    {
        return;
    }

    if (node->childs_.empty())
    {
        ws.clear();
        node->fullPath(ws);
        node->size_ = detail::tryGetFileSize(ws);

        return;
    }

    node->size_ = 0;
    node->sha256_.clear();

    for (auto& it : node->childs_)
    {
        Node* child = it.second.get();

        updateHelper(child, ws);
        node->size_ += child->size();
    }
}
