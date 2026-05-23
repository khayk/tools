
#include <gtest/gtest.h>

#include <duplicates/DuplicateOperation.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Config.h>
#include <duplicates/Progress.h>
#include <core/utils/File.h>
#include <core/utils/LogCapture.h>
#include <filesystem>

using namespace core;

namespace tools::dups {

namespace {

void writeFiles(const fs::path& dir)
{
    file::write(dir / "a.txt", "hello");
    file::write(dir / "b.txt", "hello");
    file::write(dir / "c.txt", "world");
    file::write(dir / "d.txt", "world");
    file::write(dir / "e.txt", "unique");
}

} // namespace

class DuplicateOperationTest : public ::testing::Test
{
    void SetUp() override
    {
        fs::create_directories(cfg.dataDir());
        fs::create_directories(cfg.cacheDir());
        fs::create_directories(scanDir);
        applyDefaults(cfg);
    }

protected:
    utl::LogCaptureSt log;
    file::TempDir tmp{"dups"};
    fs::path scanDir{tmp.path() / "scan"};
    Config cfg{tmp.path() / "data", tmp.path() / "cache"};
    Progress progress{nullptr};
};

TEST_F(DuplicateOperationTest, ScanDirectoriesAddsFiles)
{
    DuplicateDetector detector;

    writeFiles(scanDir);
    cfg.addScanDir(scanDir);

    scanDirectories(cfg, detector, progress);

    EXPECT_EQ(detector.numFiles(), 5);
    EXPECT_TRUE(log.contains("Discovered files: 5"));
}

TEST_F(DuplicateOperationTest, DetectDuplicatesSkipsWhenConfigured)
{
    DuplicateDetector detector;

    cfg.setSkipDetection(true);
    cfg.addScanDir(scanDir);

    writeFiles(scanDir);
    detectDuplicates(cfg, detector, progress);

    EXPECT_EQ(detector.numGroups(), 0);
    EXPECT_TRUE(log.contains("Skip duplicate detection"));
}

TEST_F(DuplicateOperationTest, DetectDuplicatesFindsDuplicateGroups)
{
    DuplicateDetector detector;

    cfg.addScanDir(scanDir);
    writeFiles(scanDir);

    scanDirectories(cfg, detector, progress);
    detectDuplicates(cfg, detector, progress);

    EXPECT_EQ(detector.numGroups(), 2);
}

TEST_F(DuplicateOperationTest, OutputFilesSkipsWhenPathEmpty)
{
    DuplicateDetector detector;

    outputFiles({}, detector);

    EXPECT_TRUE(log.contains("Skip dumping"));
}

TEST_F(DuplicateOperationTest, OutputFilesWritesScannedPaths)
{
    DuplicateDetector detector;

    cfg.addScanDir(scanDir);
    writeFiles(scanDir);

    scanDirectories(cfg, detector, progress);
    outputFiles(cfg.allFilesPath(), detector);

    const auto& out = cfg.allFilesPath();
    EXPECT_TRUE(fs::exists(out));
    EXPECT_GT(fs::file_size(out), 0);
}

TEST_F(DuplicateOperationTest, ReportDuplicatesWritesGroupEntries)
{
    DuplicateDetector detector;

    cfg.addScanDir(scanDir);

    writeFiles(scanDir);
    scanDirectories(cfg, detector, progress);
    detectDuplicates(cfg, detector, progress);

    const auto& out = cfg.allFilesPath();
    reportDuplicates(out, detector);

    EXPECT_TRUE(fs::exists(out));
    EXPECT_GT(fs::file_size(out), 0);
}

} // namespace tools::dups
