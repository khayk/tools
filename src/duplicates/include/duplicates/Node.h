#pragma once

#include <functional>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace tools::dups {

class Node
{
public:
    using UpdateCallback = std::function<void(const Node*)>;
    using ConstNodeCallback = std::function<void(const Node*)>;
    using MutableNodeCallback = std::function<void(Node*)>;
    using Children = std::unordered_map<std::wstring_view, std::unique_ptr<Node>>;

    explicit Node(std::wstring_view name, Node* parent = nullptr);

    std::wstring_view name() const noexcept;
    Node* parent() const noexcept;
    bool leaf() const noexcept;
    size_t size() const noexcept;
    uint16_t depth() const noexcept;
    const std::string& sha256() const;

    std::wstring fullPath() const;
    void fullPath(std::wstring& ws) const;

    bool hasChild(std::wstring_view name) const;
    Node* addChild(std::wstring_view name);

    void enumLeafs(const ConstNodeCallback& cb) const;
    void enumLeafs(const MutableNodeCallback& cb);
    void enumNodes(const ConstNodeCallback& cb) const;

    size_t nodesCount() const noexcept;
    size_t leafsCount() const noexcept;

    /**
     * @brief Update the given node, it's descendants and propagate all the changes up
     *
     * @param cb Delivers updates about the progress of the update
     */
    void update(const UpdateCallback& cb = [](const Node*) {});

private:
    Children children_;
    mutable std::string sha256_;
    std::wstring_view name_;
    Node* parent_ {nullptr};
    size_t size_ {0};
    uint16_t depth_ {0};

    void updateHelper(const UpdateCallback& cb, Node* node, std::wstring& ws);
};

} // namespace tools::dups
