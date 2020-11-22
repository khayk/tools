#pragma once

#include <functional>
#include <memory>
#include <string>
#include <map>

class Node
{
public:
    Node(std::wstring_view name, Node* parent = nullptr);

    std::wstring_view name() const noexcept;
    Node* parent() const noexcept;
    bool leaf() const noexcept;
    size_t size() const noexcept;
    const std::string& sha256() const;

    std::wstring fullPath() const;
    void fullPath(std::wstring& ws) const;

    bool hasChild(std::wstring_view name) const;
    Node* addChild(std::wstring_view name);

    using ConstNodeCallback = std::function<void(const Node*)>;
    void enumLeafs(ConstNodeCallback cb) const;

    using MutableNodeCallback = std::function<void(Node*)>;
    void enumLeafs(MutableNodeCallback cb);

    size_t nodesCount() const noexcept;
    size_t leafsCount() const noexcept;

    /**
     * @brief  Update the given node, it's decendents and propogate all the changes up
     */
    void update();

private:
    std::map<std::wstring_view, std::unique_ptr<Node>> childs_;
    mutable std::string sha256_;
    std::wstring_view name_;
    Node* parent_ {nullptr};
    size_t size_ {0};

    void updateHelper(Node* node, std::wstring& ws);
};

