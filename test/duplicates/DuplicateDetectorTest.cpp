#include <gtest/gtest.h>
#include <duplicates/DuplicateDetector.h>
#include <unordered_map>

using namespace tools::dups;

TEST(DuplicateDetectorTest, AddFiles)
{
    DuplicateDetector dd;
    std::unordered_map<fs::path, bool> files {{"a/b/c.txt", false},
                                              {"a/d", false},
                                              {"e/.f", false}};

    EXPECT_EQ(0, dd.numFiles());

    for (const auto& [path, _] : files)
    {
        const auto numFiles = dd.numFiles();
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles());
    
        // An extra call is to show that adding an existing path has no
        // side effect at all
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles()); 
    }

    EXPECT_EQ(files.size(), dd.numFiles());

    dd.enumFiles([&files](const fs::path& p) {
        auto it = files.find(p);

        ASSERT_TRUE(it != files.end());
        EXPECT_EQ(it->first, p);
        EXPECT_EQ(it->second, false);
        it->second = true;
    });
}

TEST(DuplicateDetectorTest, BasicUsage)
{
}

TEST(DuplicateDetectorTest, AdvancedUsage)
{
}