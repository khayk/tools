#include <gtest/gtest.h>

#include <duplicates/Config.h>

#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <chrono>

namespace tools::dups {

// ─── construction ─────────────────────────────────────────────────────────────

TEST(ConfigTest, ConstructorAppendsDuplicatesSubdir)
{
    Config cfg("/data", "/cache");
    EXPECT_EQ(cfg.dataDir(),  fs::path("/data/duplicates"));
    EXPECT_EQ(cfg.cacheDir(), fs::path("/cache/duplicates"));
    EXPECT_EQ(cfg.logDir(),   fs::path("/data/duplicates/logs"));
}

TEST(ConfigTest, InitialStateHasEmptyCollectionsAndZeroSizes)
{
    Config cfg("/data", "/cache");
    EXPECT_TRUE(cfg.scanDirs().empty());
    EXPECT_TRUE(cfg.dirsToKeepFrom().empty());
    EXPECT_TRUE(cfg.dirsToDeleteFrom().empty());
    EXPECT_TRUE(cfg.exclusionPatterns().empty());
    EXPECT_EQ(cfg.minFileSizeBytes(), 0U);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 0U);
    EXPECT_FALSE(cfg.skipDetection());
    EXPECT_TRUE(cfg.dryRun());
}

// ─── scan dirs ───────────────────────────────────────────────────────────────

TEST(ConfigTest, AddScanDirNormalizesPath)
{
    Config cfg("/data", "/cache");
    cfg.addScanDir("/tmp/foo/../bar");
    ASSERT_EQ(cfg.scanDirs().size(), 1U);
    EXPECT_EQ(cfg.scanDirs().front(), fs::path("/tmp/bar"));
}

TEST(ConfigTest, SetScanDirsReplacesExisting)
{
    Config cfg("/data", "/cache");
    cfg.addScanDir("/a");
    cfg.setScanDirs({"/b", "/c"});
    EXPECT_EQ(cfg.scanDirs().size(), 2U);
}

// ─── keep / delete dirs ───────────────────────────────────────────────────────

TEST(ConfigTest, AddDirToKeepFrom)
{
    Config cfg("/data", "/cache");
    cfg.addDirToKeepFrom("/keep/this");
    ASSERT_EQ(cfg.dirsToKeepFrom().size(), 1U);
    EXPECT_EQ(cfg.dirsToKeepFrom().front(), fs::path("/keep/this"));
}

TEST(ConfigTest, AddDirToDeleteFrom)
{
    Config cfg("/data", "/cache");
    cfg.addDirToDeleteFrom("/delete/this");
    ASSERT_EQ(cfg.dirsToDeleteFrom().size(), 1U);
    EXPECT_EQ(cfg.dirsToDeleteFrom().front(), fs::path("/delete/this"));
}

TEST(ConfigTest, SetDirsToKeepFromReplacesExisting)
{
    Config cfg("/data", "/cache");
    cfg.addDirToKeepFrom("/old");
    cfg.setDirsToKeepFrom({"/new1", "/new2"});
    EXPECT_EQ(cfg.dirsToKeepFrom().size(), 2U);
}

// ─── exclusion patterns ───────────────────────────────────────────────────────

TEST(ConfigTest, AddExclusionPatternIncreasesCount)
{
    Config cfg("/data", "/cache");
    cfg.addExclusionPattern(".*\\.tmp");
    cfg.addExclusionPattern(".*\\.log");
    EXPECT_EQ(cfg.exclusionPatterns().size(), 2U);
}

TEST(ConfigTest, SetExclusionPatternsReplacesExisting)
{
    Config cfg("/data", "/cache");
    cfg.addExclusionPattern(".*\\.tmp");
    cfg.setExclusionPatterns({".*\\.bak"});
    EXPECT_EQ(cfg.exclusionPatterns().size(), 1U);
}

// ─── auxiliary file paths ─────────────────────────────────────────────────────

TEST(ConfigTest, RelativeFilePathPrefixedWithDataDir)
{
    Config cfg("/data", "/cache");
    cfg.setAllFilesPath("all.txt");
    EXPECT_EQ(cfg.allFilesPath(), cfg.dataDir() / "all.txt");
}

TEST(ConfigTest, AbsoluteFilePathKeptAsIs)
{
    Config cfg("/data", "/cache");
    cfg.setAllFilesPath("/absolute/all.txt");
    EXPECT_EQ(cfg.allFilesPath(), fs::path("/absolute/all.txt"));
}

TEST(ConfigTest, EmptyFilePathRemainsEmpty)
{
    Config cfg("/data", "/cache");
    cfg.setAllFilesPath("");
    EXPECT_TRUE(cfg.allFilesPath().empty());
}

// ─── scalars ─────────────────────────────────────────────────────────────────

TEST(ConfigTest, FileSizeBounds)
{
    Config cfg("/data", "/cache");
    cfg.setMinFileSizeBytes(512);
    cfg.setMaxFileSizeBytes(1024UL * 1024);
    EXPECT_EQ(cfg.minFileSizeBytes(), 512U);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 1024UL * 1024);
}

TEST(ConfigTest, UpdateFrequency)
{
    Config cfg("/data", "/cache");
    cfg.setUpdateFrequency(std::chrono::milliseconds(250));
    EXPECT_EQ(cfg.updateFrequency(), std::chrono::milliseconds(250));
}

TEST(ConfigTest, SkipDetectionAndDryRun)
{
    Config cfg("/data", "/cache");
    cfg.setSkipDetection(true);
    cfg.setDryRun(false);
    EXPECT_TRUE(cfg.skipDetection());
    EXPECT_FALSE(cfg.dryRun());
}

// ─── applyDefaults ────────────────────────────────────────────────────────────

TEST(ConfigTest, ApplyDefaultsSetsExpectedValues)
{
    Config cfg("/data", "/cache");
    applyDefaults(cfg);
    EXPECT_EQ(cfg.minFileSizeBytes(), 1024U);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 10UL * 1024 * 1024 * 1024);
    EXPECT_EQ(cfg.updateFrequency(), std::chrono::milliseconds(100));
    EXPECT_FALSE(cfg.allFilesPath().empty());
    EXPECT_FALSE(cfg.dupFilesPath().empty());
    EXPECT_FALSE(cfg.ignFilesPath().empty());
}

// ─── applyOverrides ───────────────────────────────────────────────────────────

TEST(ConfigTest, ApplyOverridesIgnoresMissingFile)
{
    Config cfg("/data", "/cache");
    applyDefaults(cfg);
    EXPECT_NO_THROW(applyOverrides("/non/existent/file.toml", cfg));
}

TEST(ConfigTest, ApplyOverridesIgnoresEmptyPath)
{
    Config cfg("/data", "/cache");
    applyDefaults(cfg);
    EXPECT_NO_THROW(applyOverrides("", cfg));
}

TEST(ConfigTest, ApplyOverridesReadsTomlValues)
{
    core::file::TempDir tmp("cfg-test");
    const auto cfgFile = tmp.path() / "test.toml";
    core::file::write(cfgFile,
        "min_file_size_bytes = 2048\n"
        "max_file_size_bytes = 999999\n"
        "dry_run = false\n"
        "scan_directories = [\"/tmp/scans\"]\n"
        "exclusion_patterns = [\"pattern\"]\n"
        "dirs_to_keep_from = []\n"
        "dirs_to_delete_from = []\n");

    Config cfg("/data", "/cache");
    applyDefaults(cfg);
    applyOverrides(cfgFile, cfg);

    EXPECT_EQ(cfg.minFileSizeBytes(), 2048U);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 999999U);
    EXPECT_FALSE(cfg.dryRun());
    EXPECT_EQ(cfg.scanDirs().size(), 1U);
    EXPECT_EQ(cfg.exclusionPatterns().size(), 1U);
}

} // namespace tools::dups
