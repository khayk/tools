#include <gtest/gtest.h>

#include <duplicates/Utils.h>
#include <duplicates/Node.h>

#include <sstream>

namespace tools::dups {
namespace {

// Builds a small tree and keeps path objects alive for the test lifetime
struct TreeFixture
{
    fs::path root {"root"};
    fs::path dir1 {"dir1"};
    fs::path dir2 {"dir2"};
    fs::path file1 {"file1.txt"};
    fs::path file2 {"file2.txt"};
    fs::path file3 {"file3.txt"};

    Node tree {&root};
};

} // namespace

TEST(UtilsTest, OutputTreeEmptyRootProducesNoOutput)
{
    TreeFixture f;
    std::ostringstream os;
    util::outputTree(&f.tree, os);
    EXPECT_TRUE(os.str().empty());
}

TEST(UtilsTest, OutputTreeSingleLeafChild)
{
    TreeFixture f;
    f.tree.addChild(f.file1);

    std::ostringstream os;
    util::outputTree(&f.tree, os);
    EXPECT_EQ(os.str(), "file1.txt\n");
}

TEST(UtilsTest, OutputTreeDirectoryWithFiles)
{
    TreeFixture f;
    Node* dir1 = f.tree.addChild(f.dir1);
    dir1->addChild(f.file1);
    dir1->addChild(f.file2);

    std::ostringstream os;
    util::outputTree(&f.tree, os);
    const std::string out = os.str();

    // dir1 is depth 1 — no trailing slash even though it has children
    EXPECT_EQ(out.substr(0, 5), "dir1\n");
    // Both files appear with one-space indent; order is unordered_map-defined
    EXPECT_NE(out.find(" file1.txt\n"), std::string::npos);
    EXPECT_NE(out.find(" file2.txt\n"), std::string::npos);
}

TEST(UtilsTest, OutputTreeNestedDirectoriesGetSlash)
{
    TreeFixture f;
    Node* dir1 = f.tree.addChild(f.dir1);
    Node* dir2 = dir1->addChild(f.dir2);
    dir2->addChild(f.file1);

    std::ostringstream os;
    util::outputTree(&f.tree, os);

    // dir1 depth 1 — no slash; dir2 depth 2, non-leaf — gets slash
    const std::string expected =
        "dir1\n"
        " dir2/\n"
        "  file1.txt\n";
    EXPECT_EQ(os.str(), expected);
}

} // namespace tools::dups
