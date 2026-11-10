#include <gtest/gtest.h>

#include <duplicates/CmdLine.h>
#include <duplicates/Config.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Restore.h>

#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <cxxopts.hpp>
#include <chrono>
#include <optional>
#include <sstream>
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

TEST(CmdLineTest, VersionOption)
{
    EXPECT_TRUE(parse({"duplicates", "--version"}).contains("version"));
    EXPECT_TRUE(parse({"duplicates", "-v"}).contains("version"));
    EXPECT_FALSE(parse({"duplicates"}).contains("version"));
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

TEST_F(SilentConfig, DryRunDefaultsToRealDeletionWithoutFlag)
{
    // Without an explicit --dry-run, the authoritative default (false) must hold.
    // Guards against the CLI default silently driving (and flipping) the value.
    auto result = parse({"duplicates"});
    populateConfig(result, cfg);
    EXPECT_FALSE(cfg.dryRun());
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

TEST_F(SilentConfig, MetricsFileFlagTriggersReview)
{
    // The hidden --metrics-file flag loads the listed paths and reports usage,
    // signalling the caller to exit.
    core::file::TempDir tmp("dups");
    core::file::write(tmp.path() / "files.txt", "\"/a/b\"\n/c/d\n");
    const std::string path = (tmp.path() / "files.txt").string();

    auto result = parse({"duplicates", "--metrics-file", path.c_str()});
    EXPECT_TRUE(runMetricsReview(result));
}

TEST_F(SilentConfig, NoMetricsFileSkipsReview)
{
    auto result = parse({"duplicates"});
    EXPECT_FALSE(runMetricsReview(result));
}

TEST_F(SilentConfig, FilePathOptions)
{
    const auto base = fs::temp_directory_path();
    const auto allPath = (base / "all.txt").string();
    const auto dupPath = (base / "dup.txt").string();
    const auto ignPath = (base / "ign.txt").string();

    auto result = parse({"duplicates",
                         "--all-files",
                         allPath.c_str(),
                         "--dup-files",
                         dupPath.c_str(),
                         "--ign-files",
                         ignPath.c_str()});
    populateConfig(result, cfg);
    EXPECT_EQ(cfg.allFilesPath(), base / "all.txt");
    EXPECT_EQ(cfg.dupFilesPath(), base / "dup.txt");
    EXPECT_EQ(cfg.ignFilesPath(), base / "ign.txt");
}

namespace {

class RestoreCommands : public testing::Test
{
protected:
    core::utl::MuteLogger mute_;
    core::file::TempDir tmp_ {"dups"};
    Config cfg_ {tmp_.path() / "data", tmp_.path() / "cache"};
    std::ostringstream out_;

    /// Create a file and delete it the way a normal run would.
    fs::path deleteFile(std::string_view name, std::string_view text)
    {
        auto file = tmp_.path() / name;
        core::file::write(file, text);
        BackupAndDelete(cfg_.cacheDir()).remove(file);
        return file;
    }

    std::optional<int> run(std::vector<const char*> args)
    {
        args.insert(args.begin(), "duplicates");
        return runRestoreCommands(parse(std::move(args)), cfg_, out_);
    }
};

} // namespace

TEST_F(RestoreCommands, NotRequested)
{
    EXPECT_EQ(run({}), std::nullopt);
    EXPECT_EQ(run({"--dry-run"}), std::nullopt);
    EXPECT_TRUE(out_.str().empty());
}

TEST_F(RestoreCommands, ListWithoutRuns)
{
    EXPECT_EQ(run({"--list-runs"}), 0);
    EXPECT_TRUE(out_.str().contains("No restorable runs"));
}

TEST_F(RestoreCommands, ListShowsRuns)
{
    deleteFile("a.txt", "alpha");

    EXPECT_EQ(run({"--list-runs"}), 0);

    const auto runs = listRuns(cfg_.cacheDir());
    ASSERT_EQ(runs.size(), 1U);
    EXPECT_TRUE(out_.str().contains(runs.front().id));
    EXPECT_TRUE(out_.str().contains("pending"));
}

TEST_F(RestoreCommands, RestoreLatest)
{
    const auto file = deleteFile("a.txt", "alpha");

    EXPECT_EQ(run({"--restore", "latest"}), 0);
    EXPECT_TRUE(fs::exists(file));
    EXPECT_TRUE(out_.str().contains("Restored: 1,"));
    EXPECT_TRUE(listRuns(cfg_.cacheDir()).empty());
}

TEST_F(RestoreCommands, RestoreById)
{
    const auto file = deleteFile("a.txt", "alpha");
    const auto id = listRuns(cfg_.cacheDir()).front().id;

    EXPECT_EQ(run({"--restore", id.c_str()}), 0);
    EXPECT_TRUE(fs::exists(file));
}

TEST_F(RestoreCommands, DryRunRestoresNothing)
{
    const auto file = deleteFile("a.txt", "alpha");
    cfg_.setDryRun(true);

    EXPECT_EQ(run({"--restore", "latest"}), 0);
    EXPECT_FALSE(fs::exists(file));
    EXPECT_TRUE(out_.str().contains("Would restore: 1,"));
    EXPECT_EQ(listRuns(cfg_.cacheDir()).size(), 1U);
}

TEST_F(RestoreCommands, IncompleteRestoreReturnsTwo)
{
    const auto file = deleteFile("a.txt", "alpha");
    core::file::write(file, "something else");

    EXPECT_EQ(run({"--restore", "latest"}), 2);

    const auto& text = out_.str();
    EXPECT_TRUE(text.contains("conflicts: 1"));
    EXPECT_TRUE(text.contains(core::file::path2s(file)));
    EXPECT_TRUE(text.contains("again"));
}

TEST_F(RestoreCommands, DryRunWithConflictReturnsZero)
{
    const auto file = deleteFile("a.txt", "alpha");
    core::file::write(file, "something else");
    cfg_.setDryRun(true);

    // The conflict is reported, but a dry run leaves nothing behind.
    EXPECT_EQ(run({"--restore", "latest"}), 0);
    EXPECT_TRUE(out_.str().contains("conflicts: 1"));
    EXPECT_FALSE(out_.str().contains("again"));
}

TEST_F(RestoreCommands, ListFailsWhenCacheDirCannotBeListed)
{
    fs::create_directories(cfg_.cacheDir().parent_path());
    core::file::write(cfg_.cacheDir(), "not a directory");

    EXPECT_THROW(run({"--list-runs"}), fs::filesystem_error);
    EXPECT_FALSE(out_.str().contains("No restorable runs"));
}

TEST_F(RestoreCommands, UnknownRunThrows)
{
    deleteFile("a.txt", "alpha");
    EXPECT_THROW(run({"--restore", "deleted_20000101_000000"}), std::runtime_error);
}

} // namespace tools::dups
