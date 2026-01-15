#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <core/utils/File.h>
#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <cstddef>
#include <queue>
#include <unordered_map>
#include "duplicates/IDuplicates.h"
#include "duplicates/Progress.h"
#include <fmt/format.h>

using testing::MockFunction;
using testing::Return;

namespace tools::dups {
namespace {

using GenFileCb = std::function<void(const fs::path&)>;
void generateFiles(size_t numFiles,
                   size_t filesPerLevel,
                   size_t dirsPerLevel,
                   const GenFileCb& cb)
{
    std::queue<fs::path> dirs;
    dirs.emplace("top");

    while (true)
    {
        if (dirs.empty())
        {
            break;
        }

        const fs::path& dir = dirs.front();

        for (size_t i = 0; i < dirsPerLevel; ++i)
        {
            fs::path subdir = dir / fmt::format("subdir-{}", i);
            dirs.emplace(subdir);
        }

        for (size_t i = 0; i < filesPerLevel; ++i)
        {
            fs::path file = dir / fmt::format("file-{}.txt", i);
            cb(file);

            if (--numFiles == 0)
            {
                return;
            }
        }

        dirs.pop();
    }
}

using FileDataMap = std::unordered_map<fs::path, std::string>;

FileDataMap getTestFiles(const fs::path& dir)
{
    /** File structure defined below:
     *
     * u/
     *    m/
     *       l/
     *          f1
     *          ф2
     *          ֆ3
     *    f1-dup1
     *    f1-dup2
     *    f2-dup1
     * f1-dup3
     * f2-dup2
     * f3-dup1
     * f4-uniq
     */

    return {{dir / "u/m/l/f1", "1"},
            {dir / u8"u/m/l/ф2", "22"},  // <- unicode filename
            {dir / u8"u/m/l/ֆ3", "333"}, // <- unicode filename
            {dir / "u/m/f1-dup1", "1"},
            {dir / "u/m/f1-dup2", "1"},
            {dir / "u/m/f2-dup1", "22"},
            {dir / "u/f1-dup3", "1"},
            {dir / "u/f2-dup2", "22"},
            {dir / "u/f3-dup1", "333"},
            {dir / "u/f4-uniq", "4444"}};
}

void createFiles(const FileDataMap& files)
{
    for (const auto& [file, content] : files)
    {
        fs::create_directories(file.parent_path());
        file::write(file, content);
    }
}

void addFiles(const FileDataMap& files, DuplicateDetector& dd)
{
    for (const auto& [file, _] : files)
    {
        dd.addFile(file);
    }
}

} // namespace

TEST(DuplicateDetectorTest, AddFiles)
{
    DuplicateDetector dd;

    // These paths doesn't have to be existing files
    std::unordered_map<fs::path, bool> files {{"/a/b/c.txt", false},
                                              {"/a/d", false},
                                              {"/e/.f", false}};

    EXPECT_EQ(0, dd.numFiles());

    for (const auto& [path, seen] : files)
    {
        const auto numFiles = dd.numFiles();
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles());

        // An extra call with an existing path has no side effect
        dd.addFile(path);
        EXPECT_EQ(numFiles + 1, dd.numFiles());

        EXPECT_FALSE(seen);
    }

    EXPECT_EQ(files.size(), dd.numFiles());
    MockFunction<void(const fs::path&)> fileCb;

    // Expect that each file is enumerated exactly once
    EXPECT_CALL(fileCb, Call(testing::_))
        .Times(static_cast<int>(files.size()))
        .WillRepeatedly([&files](const fs::path& p) {
            auto it = files.find(p);
            ASSERT_TRUE(it != files.end());

            auto& [file, seen] = *files.find(p);
            EXPECT_EQ(file, p);
            EXPECT_FALSE(seen);
            seen = true;
        });

    dd.enumFiles([&fileCb](const fs::path& p) {
        fileCb.Call(p);
    });
}


TEST(DuplicateDetectorTest, DetectDuplicates)
{
    file::TempDir data("dups");
    DuplicateDetector dd;
    IDuplicateDetector& idd = dd;
    Options opts;

    const auto files = getTestFiles(data.path());
    createFiles(files);
    addFiles(files, dd);

    EXPECT_EQ(files.size(), dd.numFiles());
    MockFunction<void(const DupGroup&)> dupCb;

    // Expect no duplicates before detection
    EXPECT_CALL(dupCb, Call(testing::_)).Times(0);
    dd.enumGroups([&dupCb](const DupGroup& grp) {
        dupCb.Call(grp);
        return true;
    });

    // Expect 3 duplicate group after detection, each with at least 2 entries
    EXPECT_CALL(dupCb, Call(testing::_))
        .Times(3)
        .WillRepeatedly([](const DupGroup& grp) {
            EXPECT_GE(grp.entires.size(), 2);
        });

    idd.detect(opts);
    dd.enumGroups([&dupCb](const DupGroup& grp) {
        dupCb.Call(grp);
        return true;
    });
}


TEST(DuplicateDetectorTest, MetricsThresholds)
{
    constexpr size_t numFiles = 50'000;
    constexpr size_t filesPerLevel = 10;
    constexpr size_t dirsPerLevel = 2;

    StopWatch sw;
    DuplicateDetector detector;
    generateFiles(numFiles,
                  filesPerLevel,
                  dirsPerLevel,
                  [&detector](const fs::path& file) {
                      detector.addFile(file);
                  });

    EXPECT_EQ(detector.numFiles(), numFiles);
    EXPECT_LE(sizeof(Node), 120);
    EXPECT_LE(sys::currentProcessMemoryUsage(), 27 * 1024 * 1024);
}

TEST(DuplicateDetectorTest, Progress)
{
    std::ostringstream oss;
    Progress progress(&oss, std::chrono::milliseconds(0));
    size_t totalCalls = 0;

    for (int i = 1; i <= 3; ++i)
    {
        progress.update([i, &totalCalls](std::ostream& os) {
            os << "Test progress " << i;
            ++totalCalls;
        });
    }

    EXPECT_EQ(totalCalls, 3);
    EXPECT_EQ(oss.str(), "Test progress 1\rTest progress 2\rTest progress 3\r");

    progress.setFrequency(std::chrono::milliseconds(100));
    oss.str("");

    progress.update([&totalCalls](std::ostream& os) {
        os << "This should not be called";
        ++totalCalls;
    });

    EXPECT_EQ(totalCalls, 3);
    EXPECT_EQ(oss.str(), "");
}

} // namespace tools::dups
