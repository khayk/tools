#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DuplicateDetector.h>
#include <core/utils/File.h>

#include <unordered_map>

using testing::MockFunction;
using testing::Return;
using testing::_;

namespace tools::dups {
namespace {

void createFiles(const std::unordered_map<fs::path, std::string>& files,
                 const fs::path& dir)
{
    for (const auto& [file, content] : files)
    {
        fs::create_directories(dir / file.parent_path());
        file::write(dir / file, content);
    }
}

} // namespace

TEST(DuplicateDetectorTest, AddFiles)
{
    DuplicateDetector dd;
    std::unordered_map<fs::path, bool> files {{"/a/b/c.txt", false},
                                              {"/a/d", false},
                                              {"/e/.f", false}};

    EXPECT_EQ(0, dd.numFiles());

    for (const auto& [path, ignore]: files)
    {
        std::ignore = ignore;
        const auto numFiles = dd.numFiles();
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles());

        // An extra call is to show that adding an existing path has no
        // side effect at all
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles());
    }

    EXPECT_EQ(files.size(), dd.numFiles());
    MockFunction<void(const fs::path&)> fileCb;

    EXPECT_CALL(fileCb, Call(_))
        .Times(static_cast<int>(files.size()))
        .WillRepeatedly([&files](const fs::path& p) {
            auto it = files.find(p);

            ASSERT_TRUE(it != files.end());
            EXPECT_EQ(it->first, p);
            EXPECT_EQ(it->second, false);
            it->second = true;
        });

    dd.enumFiles([&fileCb](const fs::path& p) {
        fileCb.Call(p);
    });
}

TEST(DuplicateDetectorTest, BasicUsage)
{
    file::TempDir data("dups");

    const std::unordered_map<fs::path, std::string> files {
        {"a/d.txt", "1"},
        {"b/e.jpg", "1"},
        {"c/f.dat", "2"},
        {"c/g.pdf", "1"},
        {"h", "2"},
    };

    createFiles(files, data.path());

    DuplicateDetector dd;
    DuplicateDetector::Options opts;

    for (const auto& [file, content] : files)
    {
        dd.addFile(data.path() / file);
    }

    EXPECT_EQ(files.size(), dd.numFiles());
    MockFunction<void(const DupGroup&)> dupCb;

    EXPECT_CALL(dupCb, Call(_)).Times(0);
    dd.enumDuplicates([&dupCb](const DupGroup& grp) {
        dupCb.Call(grp);
    });


    EXPECT_CALL(dupCb, Call(_)).Times(2).WillRepeatedly([](const DupGroup& grp) {
        EXPECT_GE(grp.entires.size(), 2);
    });

    dd.detect(opts);
    dd.enumDuplicates([&dupCb](const DupGroup& grp) {
        dupCb.Call(grp);
    });
}


TEST(DuplicateDetectorTest, AdvancedUsage)
{
}

} // namespace tools::dups
