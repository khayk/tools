#include <gtest/gtest.h>

#include <duplicates/CmdLine.h>
#include <duplicates/Config.h>

#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <cxxopts.hpp>
#include <chrono>
#include <vector>

namespace tools::dups {
namespace {

cxxopts::ParseResult parse(std::vector<const char*> args)
{
    cxxopts::Options opts("duplicates", "test");
    defineOptions(opts);
    return opts.parse(static_cast<int>(args.size()), args.data());
}

// Suppress the "missing config file" spdlog warning that populateConfig
// emits when the default dups.toml doesn't exist.
struct SilentConfig : testing::Test
{
    core::utl::MuteLogger mute;
    Config cfg {"/data", "/cache"};
};

} // namespace

TEST(CmdLineTest, DefineOptionsDoesNotThrow)
{
    cxxopts::Options opts("duplicates", "test");
    EXPECT_NO_THROW(defineOptions(opts));
}

TEST_F(SilentConfig, PopulateConfigAppliesDefaultSizes)
{
    auto result = parse({"duplicates"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.minFileSizeBytes(), 1U);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 10UL * 1024 * 1024 * 1024);
    EXPECT_EQ(cfg.updateFrequency(), std::chrono::milliseconds(100));
}

TEST_F(SilentConfig, ScanDirOption)
{
    auto result =
        parse({"duplicates", "--scan-dir", "/tmp/a", "--scan-dir", "/tmp/b"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.scanDirs().size(), 2U);
}

TEST_F(SilentConfig, DryRunOption)
{
    auto result = parse({"duplicates", "--dry-run"});
    populateConfig(result, cfg);
    EXPECT_TRUE(cfg.dryRun());
}

TEST_F(SilentConfig, MinSizeOption)
{
    auto result = parse({"duplicates", "--min-size", "4096"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.minFileSizeBytes(), 4096U);
}

TEST_F(SilentConfig, MaxSizeOption)
{
    auto result = parse({"duplicates", "--max-size", "1000000"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.maxFileSizeBytes(), 1'000'000U);
}

TEST_F(SilentConfig, UpdateFreqOption)
{
    auto result = parse({"duplicates", "--update-freq", "500"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.updateFrequency(), std::chrono::milliseconds(500));
}

TEST_F(SilentConfig, ExcludeOption)
{
    auto result = parse({"duplicates", "--exclude", "pat1", "--exclude", "pat2"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.exclusionPatterns().size(), 2U);
}

TEST_F(SilentConfig, KeepAndDeletePathOptions)
{
    auto result =
        parse({"duplicates", "--keep-path", "/keep/a", "--delete-path", "/del/b"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.dirsToKeepFrom().size(), 1U);
    EXPECT_EQ(cfg.dirsToDeleteFrom().size(), 1U);
}

TEST_F(SilentConfig, CfgFileTomlOption)
{
    // Explicitly passing --cfg-file with .toml triggers applyOverrides
    // (file absent → warning is muted by fixture)
    auto result = parse({"duplicates", "--cfg-file", "custom.toml"});
    EXPECT_NO_THROW(populateConfig(result, cfg));
}

TEST_F(SilentConfig, CfgFileNonTomlTriggersMetricsReview)
{
    // Non-.toml cfg-file triggers metricsReview and early return
    core::file::TempDir tmp("dups");
    core::file::write(tmp.path() / "files.txt", "/a/b\n/c/d\n");
    const std::string path = (tmp.path() / "files.txt").string();

    auto result = parse({"duplicates", "--cfg-file", path.c_str()});
    EXPECT_NO_THROW(populateConfig(result, cfg));
}

TEST_F(SilentConfig, FilePathOptions)
{
    auto result = parse({"duplicates",
                         "--all-files", "/out/all.txt",
                         "--dup-files", "/out/dup.txt",
                         "--ign-files", "/out/ign.txt"});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.allFilesPath(), fs::path("/out/all.txt"));
    EXPECT_EQ(cfg.dupFilesPath(), fs::path("/out/dup.txt"));
    EXPECT_EQ(cfg.ignFilesPath(), fs::path("/out/ign.txt"));
}

} // namespace tools::dups
