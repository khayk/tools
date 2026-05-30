#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DeletionStrategy.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/utils/Log.h>
#include <core/utils/LogCapture.h>
#include <memory>
#include <array>

namespace tools::dups {

using namespace core;
using utl::MuteLogger;
using utl::LogCaptureSt;

TEST(DeletionStrategyTest, PermanentDelete)
{
    MuteLogger mute;
    file::TempDir data("dups");
    PermanentDelete strategy;

    const fs::path file = data.path() / "test.txt";
    file::write(file, "test content");

    EXPECT_TRUE(fs::exists(file));
    strategy.remove(file);
    EXPECT_FALSE(fs::exists(file));
}

TEST(DeletionStrategyTest, BackupAndDelete)
{
    MuteLogger mute;
    file::TempDir data("dups");
    const auto backupDir = data.path() / "backup";
    auto strategy = std::make_unique<BackupAndDelete>(backupDir);
    const auto journalFile = strategy->journalFile();
    const auto runDir = strategy->runDir();

    // create journal file when there is something to write to it
    EXPECT_FALSE(fs::exists(journalFile));

    const std::array files {
        data.path() / "test1.txt",
        data.path() / "test2.txt",
        data.path() / "test3.txt",
    };
    constexpr auto sampleContent = "test content";

    for (const auto& file : files)
    {
        file::write(file, sampleContent);
        EXPECT_TRUE(fs::exists(file));
        strategy->remove(file);
        EXPECT_FALSE(fs::exists(file));

        // Should be no-op, as the file already deleted
        strategy->remove(file);

        const auto parentPath = files.front().parent_path();
        const auto hash = crypto::md5(parentPath.string());

        EXPECT_TRUE(fs::exists(journalFile));
        EXPECT_TRUE(fs::exists(runDir / hash / file.filename()));
    }

    // Backups and journal live under a per-run directory beneath the backup root
    EXPECT_EQ(runDir.parent_path(), backupDir);
    EXPECT_TRUE(runDir.filename().string().starts_with("deleted_"));
    EXPECT_EQ(journalFile.filename(), "deleted_files.log");
    strategy.reset(); // This will force the journal file to be closed

    file::readLines(
        journalFile,
        [it = files.begin()](const std::string& line) mutable {
            EXPECT_FALSE(line.empty());
            EXPECT_TRUE(line.contains(file::path2s(it->filename()).c_str()));
            ++it;
            return true;
        });
}

TEST(DeletionStrategyTest, BackupsFromSeparateRunsDoNotCollide)
{
    MuteLogger mute;
    file::TempDir data("dups");
    const auto backupDir = data.path() / "backup";
    const fs::path file = data.path() / "photo.jpg";
    const auto hash = crypto::md5(file.parent_path().string());

    // Run 1: create the file and delete it.
    file::write(file, "version one");
    fs::path backup1;
    {
        BackupAndDelete run1(backupDir);
        run1.remove(file);
        backup1 = run1.runDir() / hash / file.filename();
        EXPECT_TRUE(fs::exists(backup1));
    }

    // Run 2: the same path is recreated with different content and deleted again.
    file::write(file, "version two");
    fs::path backup2;
    {
        BackupAndDelete run2(backupDir);
        run2.remove(file);
        backup2 = run2.runDir() / hash / file.filename();
        EXPECT_TRUE(fs::exists(backup2));
    }

    // Distinct run directories => the first backup is preserved intact instead of
    // being silently overwritten by the second run.
    ASSERT_NE(backup1, backup2);
    ASSERT_TRUE(fs::exists(backup1));

    std::string content;
    std::error_code ec;
    ASSERT_TRUE(file::read(backup1, content, ec));
    EXPECT_EQ(content, "version one");
}

TEST(DeletionStrategyTest, ConstructionThrowsWhenRunDirCannotBeCreated)
{
    MuteLogger mute;
    file::TempDir data("dups");

    // Place the backup root underneath a regular file: the run directory can
    // never be created, so construction must fail fast with an error rather than
    // looping forever trying suffixed names.
    const auto blocker = data.path() / "blocker";
    file::write(blocker, "not a directory");
    const auto backupDir = blocker / "backup";

    EXPECT_THROW(BackupAndDelete {backupDir}, std::system_error);
}

TEST(DeletionStrategyTest, DryRunDelete)
{
    LogCaptureSt log;
    file::TempDir data("dups");
    DryRunDelete strategy;

    const fs::path file = data.path() / "test.txt";
    file::write(file, "test content");

    EXPECT_TRUE(fs::exists(file));
    strategy.remove(file);
    EXPECT_TRUE(fs::exists(file));
    EXPECT_TRUE(log.contains("Would delete: "));
}

} // namespace tools::dups