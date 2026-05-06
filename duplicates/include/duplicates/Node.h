#pragma once

#include <functional>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace tools::dups {

namespace fs = std::filesystem;

using NodePtr = std::unique_ptr<class Node>;

class Node
{
public:
    using UpdateCallback = std::function<void(const Node*)>;
    using ConstNodeCallback = std::function<void(const Node*)>;
    using MutableNodeCallback = std::function<void(Node*)>;
    using Children = std::unordered_map<const fs::path*, NodePtr>;

    explicit Node(const fs::path* name, Node* parent = nullptr);

    const fs::path& name() const noexcept;
    bool leaf() const noexcept;
    size_t size() const noexcept;
    uint16_t depth() const noexcept;
    const std::string& sha256() const;

    fs::path fullPath() const;
    void fullPath(fs::path& path) const;

    bool hasChild(const fs::path& name) const;
    Node* addChild(const fs::path& name);
    Node* parent() const noexcept;

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
    const fs::path* name_ {nullptr};
    Node* parent_ {nullptr};
    size_t size_ {0};
    uint16_t depth_ {0};

    void updateHelper(const UpdateCallback& cb, Node* node, fs::path& p);
};

} // namespace tools::dups
