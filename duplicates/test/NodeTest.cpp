#include <gtest/gtest.h>

#include <duplicates/Node.h>

#include <core/utils/File.h>

#include <filesystem>
#include <vector>

namespace tools::dups {
namespace {

class NodeTest : public testing::Test
{
protected:
    // Paths must outlive nodes because Node stores raw pointers to them
    fs::path rootName {"root"};
    fs::path dir1Name {"dir1"};
    fs::path dir2Name {"dir2"};
    fs::path file1Name {"file1"};
    fs::path file2Name {"file2"};
    fs::path file3Name {"file3"};
};

} // namespace

TEST_F(NodeTest, RootNodeHasNoParentAndDepthZero)
{
    Node root(&rootName);
    EXPECT_EQ(root.name(), rootName);
    EXPECT_EQ(root.parent(), nullptr);
    EXPECT_EQ(root.depth(), 0);
    EXPECT_EQ(root.size(), 0);
}

TEST_F(NodeTest, RootNodeIsInitiallyALeaf)
{
    Node root(&rootName);
    EXPECT_TRUE(root.leaf());
    EXPECT_EQ(root.nodesCount(), 1);
    EXPECT_EQ(root.leafsCount(), 1);
}

TEST_F(NodeTest, RootNodeFullPathIsEmpty)
{
    Node root(&rootName);
    EXPECT_TRUE(root.fullPath().empty());
}

TEST_F(NodeTest, ChildInheritsCorrectDepthAndParent)
{
    Node root(&rootName);
    Node* child = root.addChild(dir1Name);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->parent(), &root);
    EXPECT_EQ(child->depth(), 1);

    Node* grandchild = child->addChild(dir2Name);
    ASSERT_NE(grandchild, nullptr);
    EXPECT_EQ(grandchild->parent(), child);
    EXPECT_EQ(grandchild->depth(), 2);
}

TEST_F(NodeTest, AddingChildMakesParentNonLeaf)
{
    Node root(&rootName);
    EXPECT_TRUE(root.leaf());
    root.addChild(dir1Name);
    EXPECT_FALSE(root.leaf());
}

TEST_F(NodeTest, AddChildIsIdempotent)
{
    Node root(&rootName);
    Node* first = root.addChild(dir1Name);
    Node* second = root.addChild(dir1Name);
    EXPECT_EQ(first, second);
    EXPECT_EQ(root.nodesCount(), 2);
}

TEST_F(NodeTest, HasChildReturnsTrueAfterAdd)
{
    Node root(&rootName);
    EXPECT_FALSE(root.hasChild(dir1Name));
    root.addChild(dir1Name);
    EXPECT_TRUE(root.hasChild(dir1Name));
    EXPECT_FALSE(root.hasChild(dir2Name));
}

TEST_F(NodeTest, NodesCountIncludesSelf)
{
    Node root(&rootName);
    EXPECT_EQ(root.nodesCount(), 1);

    root.addChild(dir1Name);
    EXPECT_EQ(root.nodesCount(), 2);

    root.addChild(dir2Name);
    EXPECT_EQ(root.nodesCount(), 3);
}

TEST_F(NodeTest, LeafsCountForTree)
{
    Node root(&rootName);
    EXPECT_EQ(root.leafsCount(), 1);

    Node* dir1 = root.addChild(dir1Name);
    EXPECT_EQ(root.leafsCount(), 1);  // dir1 replaces root as the only leaf

    dir1->addChild(file1Name);
    dir1->addChild(file2Name);
    EXPECT_EQ(root.leafsCount(), 2);

    root.addChild(dir2Name);
    EXPECT_EQ(root.leafsCount(), 3);
}

TEST_F(NodeTest, FullPathReflectsHierarchy)
{
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    Node* file1 = dir1->addChild(file1Name);

    EXPECT_EQ(dir1->fullPath(), dir1Name);
    EXPECT_EQ(file1->fullPath(), dir1Name / file1Name);
}

TEST_F(NodeTest, FullPathOverloadMatchesReturn)
{
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    Node* file1 = dir1->addChild(file1Name);

    fs::path out;
    file1->fullPath(out);
    EXPECT_EQ(file1->fullPath(), out);
}

TEST_F(NodeTest, EnumLeafsConstVisitsOnlyLeaves)
{
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    dir1->addChild(file1Name);
    dir1->addChild(file2Name);
    root.addChild(dir2Name);  // leaf sibling of dir1

    std::vector<const Node*> visited;
    const Node& croot = root;
    croot.enumLeafs([&visited](const Node* n) {
        visited.push_back(n);
    });

    EXPECT_EQ(visited.size(), 3U);
    for (const Node* n : visited)
    {
        EXPECT_TRUE(n->leaf());
    }
}

TEST_F(NodeTest, EnumLeafsMutableVisitsOnlyLeaves)
{
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    dir1->addChild(file1Name);
    dir1->addChild(file2Name);

    size_t count = 0;
    root.enumLeafs([&count](Node* n) {
        EXPECT_TRUE(n->leaf());
        ++count;
    });
    EXPECT_EQ(count, 2U);
}

TEST_F(NodeTest, EnumNodesSkipsRoot)
{
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    dir1->addChild(file1Name);

    std::vector<const Node*> visited;
    root.enumNodes([&visited](const Node* n) {
        visited.push_back(n);
    });

    EXPECT_EQ(visited.size(), 2U);
    for (const Node* n : visited)
    {
        EXPECT_NE(n, &root);
    }
}

TEST_F(NodeTest, EnumNodesNonLeafsVisitedBeforeLeafs)
{
    // Tree: root -> dir1 (non-leaf) -> {file1, file2}
    //             -> file3 (leaf)
    // Expected enumNodes order: dir1, file1, file2, file3
    Node root(&rootName);
    Node* dir1 = root.addChild(dir1Name);
    dir1->addChild(file1Name);
    dir1->addChild(file2Name);
    root.addChild(file3Name);

    std::vector<bool> isLeafOrder;
    root.enumNodes([&isLeafOrder](const Node* n) {
        isLeafOrder.push_back(n->leaf());
    });

    ASSERT_EQ(isLeafOrder.size(), 4U);
    EXPECT_FALSE(isLeafOrder.front());  // dir1 visited first (non-leaf)
    EXPECT_TRUE(isLeafOrder.back());    // file3 visited last (leaf at root level)
}

TEST_F(NodeTest, UpdatePopulatesSizeFromDisk)
{
    core::file::TempDir tmp("node-update");

    core::file::write(tmp.path() / "a.txt", "hello");   // 5 bytes
    core::file::write(tmp.path() / "b.txt", "world!");  // 6 bytes

    // fullPath() skips the root's own name (root has no parent), so a direct
    // child of root contributes its name as the path prefix. Using the tmp
    // directory path as that child's name gives leaf nodes their absolute path.
    const fs::path& tmpName = tmp.path();
    fs::path aName {"a.txt"};
    fs::path bName {"b.txt"};

    Node root(&rootName);
    Node* dir = root.addChild(tmpName);
    dir->addChild(aName);
    dir->addChild(bName);

    root.update();

    EXPECT_EQ(dir->size(), 11U);   // 5 + 6
    EXPECT_EQ(root.size(), 11U);   // propagated up
}

} // namespace tools::dups
